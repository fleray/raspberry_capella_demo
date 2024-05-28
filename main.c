#include <cbl/CouchbaseLite.h>
#include <fleece/FLExpert.h>
#include <time.h>
#include <float.h>
#include <inttypes.h>
/* For uuid_generate() and uuid_unparse() */
#include <uuid/uuid.h>
#include "sensor_simulator.h"
#ifdef _MSC_VER
#include <direct.h>
#include <Shlwapi.h>

void usleep(unsigned int us)
{
    Sleep(us / 1000);
}
#else
#include <unistd.h>
#endif

static CBLDatabase *kDatabase;

#define NUM_PROBES 4

// Helper for stop replicator in the code snippet
static void stop_replicator(CBLReplicator *replicator)
{
    CBLReplicator_Stop(replicator);
    while (CBLReplicator_Status(replicator).activity != kCBLReplicatorStopped)
    {
        printf("Waiting for replicator to stop...");
        usleep(200000);
    }
    CBLReplicator_Release(replicator);
}

static void create_new_database()
{
    // tag::new-database[]
    // NOTE: No error handling, for brevity (see getting started)

    CBLError err;
    CBLDatabase *db = CBLDatabase_Open(FLSTR("my-database"), NULL, &err);
    // end::new-database[]

    kDatabase = db;
}

static void close_database()
{
    CBLDatabase *db = kDatabase;
    // tag::close-database[]
    // NOTE: No error handling, for brevity (see getting started)

    CBLError err;
    CBLDatabase_Close(db, &err);
    // end::close-database[]
}

static char *get_uuid()
{

    uuid_t binuuid;

    /*
     * Generate a UUID. We're not done yet, though,
     * for the UUID generated is in binary format
     * (hence the variable name). We must 'unparse'
     * binuuid to get a usable 36-character string.
     */
    uuid_generate_random(binuuid);

    /*
     * uuid_unparse() doesn't allocate memory for itself, so do that with
     * malloc(). 37 is the length of a UUID (36 characters), plus '\0'.
     */
    char *uuid = malloc(37);
    uuid_unparse_upper(binuuid, uuid);

    return uuid;
}

static unsigned long get_current_timestamp()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return (unsigned long)((tv.tv_sec) * 1000 + (tv.tv_usec) / 1000); // convert tv_sec & tv_usec to millisecond
}

static void add_log(char *message)
{
    unsigned long tus = get_current_timestamp();

    char *uuid = get_uuid();

    char docID[50];
    char numStr[5];
    strcpy(docID, "log::");
    strcat(docID, uuid);

    CBLCollection *collection = CBLDatabase_DefaultCollection(kDatabase, NULL);

    CBLDocument *mutableDoc = CBLDocument_CreateWithID(FLStr(docID));
    FLMutableDict properties = CBLDocument_MutableProperties(mutableDoc);
    FLMutableDict_SetString(properties, FLSTR("type"), FLSTR("log"));
    FLMutableDict_SetString(properties, FLSTR("message"), FLStr(message));

    char *tgt = malloc(sizeof(char) * 37);
    char nmilliStr[20];
    strcpy(tgt, "");
    sprintf(nmilliStr, "%lu", tus);
    strcat(tgt, numStr);
    FLMutableDict_SetString(properties, FLSTR("ts"), FLStr(nmilliStr));
    CBLCollection_SaveDocument(collection, mutableDoc, NULL);
    CBLDocument_Release(mutableDoc);

    free(uuid), uuid = NULL;
}

static void add_new_json_sample(int sensorId, double *lastValue)
{
    CBLCollection *collection = CBLDatabase_DefaultCollection(kDatabase, NULL);

    unsigned long tus = get_current_timestamp();

    // tag::tojson-document[]
    char *json = generate_json_doc(tus, lastValue, sensorId);

    // FLString json = FLValue_AsString(FLValue(str2));

    char *uuid = get_uuid();

    char docID[50];
    char numStr[5];
    strcpy(docID, "sensor_id");
    sprintf(numStr, "::%d", sensorId);
    strcat(docID, numStr);
    strcat(docID, "::");
    strcat(docID, uuid);

    printf("DocID value: %s\n", docID);
    fflush(stdout);

    // Create a document and set the JSON data to the document
    CBLError err;
    CBLDocument *newDoc = CBLDocument_CreateWithID(FLStr(docID));

    CBLDocument_SetJSON(newDoc, FLStr(json), &err);

    // Save the document to the database
    CBLCollection_SaveDocument(collection, newDoc, &err);

    // Release created doc after using it
    CBLDocument_Release(newDoc);

    free(uuid), uuid = NULL;
    free(json), json = NULL;

    // Get the document from the database
    const CBLDocument *doc = CBLCollection_GetDocument(collection, FLStr(docID), &err);

    // Get document body as JSON
    FLSliceResult docJson = CBLDocument_CreateJSON(doc);
    printf("Document in JSON :: %.*s\n", (int)docJson.size, (const char *)docJson.buf);

    // Release JSON data after using it
    FLSliceResult_Release(docJson);

    // Release doc read from the database after using it
    CBLDocument_Release(doc);
    // end::tojson-document[]
}

static void select_count()
{
    CBLDatabase *database = kDatabase;
    CBLError err;

    CBLQuery *query = CBLDatabase_CreateQuery(database, kCBLN1QLLanguage,
                                              FLSTR("SELECT count(*) FROM _"), NULL, &err);

    CBLResultSet *results = CBLQuery_Execute(query, &err);
    while (CBLResultSet_Next(results))
    {
        int64_t count = FLValue_AsInt(CBLResultSet_ValueForKey(results, FLSTR("$1")));
        printf("Local DB size : %ld\n", count);
        fflush(stdout);
    }

    CBLResultSet_Release(results);
    CBLQuery_Release(query);
}

static CBLReplicator *start_replication(char *endpointURL)
{
    CBLCollection *collection = CBLDatabase_DefaultCollection(kDatabase, NULL);

    /*
     * This requires Sync Gateway running with the following config, or equivalent:
     *
     * {
     *     "log":["*"],
     *     "databases": {
     *         "db": {
     *             "server":"walrus:",
     *             "users": {
     *                 "GUEST": {"disabled": false, "admin_channels": ["*"] }
     *             }
     *         }
     *     }
     * }
     */

    // tag::replication[]
    // NOTE: No error handling, for brevity (see getting started)
    // Note: Android emulator needs to use 10.0.2.2 for localhost (10.0.3.2 for GenyMotion)

    CBLError err;
    FLString url = FLStr(endpointURL);
    CBLEndpoint *target = CBLEndpoint_CreateWithURL(url, &err);

    CBLReplicationCollection collectionConfig;
    memset(&collectionConfig, 0, sizeof(CBLReplicationCollection));
    collectionConfig.collection = collection;

    CBLReplicatorConfiguration replConfig;
    memset(&replConfig, 0, sizeof(CBLReplicatorConfiguration));
    replConfig.collectionCount = 1;
    replConfig.collections = &collectionConfig;
    replConfig.endpoint = target;
    replConfig.replicatorType = kCBLReplicatorTypePushAndPull;
    replConfig.continuous = true;

    // Optionally, configure Client Authentication
    // Here we are using to Basic Authentication,
    // Providing username and password credentials
    CBLAuthenticator *basicAuth =
        CBLAuth_CreatePassword(FLSTR("demo"),
                               FLSTR("P@ssw0rd!"));
    replConfig.authenticator = basicAuth;

    /*
        char cert_buf[10000];
        FILE *cert_file = fopen("/home/fabrice/cb_lite_c/cert.pem", "r");
        size_t read = fread(cert_buf, 1, sizeof(cert_buf), cert_file);
        replConfig.pinnedServerCertificate = (FLSlice){cert_buf, read};
    */

    CBLReplicator *replicator = CBLReplicator_Create(&replConfig, &err);
    CBLEndpoint_Free(target);
    CBLReplicator_Start(replicator, false);

    return replicator;

    // end::replication[]

    // stop_replicator(replicator);
}

// Console logging domain methods are not applicable to C

int main(int argc, char **argv)
{

    if (argc != 2)
    {
        printf("One string argument specifying the AppServices endpoint is needed.");
        return -1;
    }

    char *endpointURL = argv[1];

    printf("endpointURL value  is %s\n", endpointURL);

    create_new_database();
    CBLReplicator *replicator = start_replication(endpointURL);

    double lastValue[NUM_PROBES][1];

    for (int j = 0; j < NUM_PROBES; j++)
    {
        *lastValue[j] = -DBL_MAX;
    }

    for (;;)
    {

        for (int i = 1; i <= NUM_PROBES; i++)
        {
            // simulate 2 probles
            int sensorId = i;
            add_new_json_sample(sensorId, lastValue[i - 1]);
            // usleep(125000);
        }
        usleep(1000000);
        select_count();

        CBLReplicatorStatus thisState = CBLReplicator_Status(replicator);

        switch (thisState.activity)
        {
        case kCBLReplicatorOffline:
            add_log("The replicator is offline, as the remote host is unreachable.");
            break;

        case kCBLReplicatorConnecting:
            add_log("The replicator is connecting to the remote host.");
            break;

        // case kCBLReplicatorIdle:
        //     add_log("The replicator is inactive, waiting for changes to sync.");
        //     break;
        case kCBLReplicatorStopped:
            add_log("The replicator is unstarted, finished, or hit a fatal error.");
            if (thisState.error.code == 0)
            {
                CBLReplicator_Start(replicator, false);
            }
            else
            {
                char code[20];
                char numStr[5];
                strcpy(code, "Code : ");
                sprintf(numStr, "%d", thisState.error.code);
                strcat(code, numStr);
                add_log(numStr);
                printf("Replicator stopped -- code %d", thisState.error.code);
                // ... handle error ...
                CBLReplicator_Release(replicator);
            }
            break;
        default:
        }
    }

    close_database();

    return 0;
}