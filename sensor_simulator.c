#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>

#define SIZE 1024

void generate_value(double *lastValue)
{
    double newValue = 0.;
    double random = (double)rand() / (double)((unsigned)RAND_MAX + 1);

    if ((*lastValue) < (-DBL_MAX / 2))
    {
        newValue = random * 40 - 10;
    }
    else
    {
        newValue = *lastValue + random * 2 - 1;
    }

    if (!(newValue > 50 || newValue < -20))
    {
        *lastValue = newValue;
    }
}

// Generate JSON document
char *generate_json_doc(unsigned long millis, double *lastValue, int sensorId)
{
    char *target = (char *)malloc(SIZE * sizeof(char));
    generate_value(lastValue);

    char numStr[20];
    strcpy(target, "{\"type\":\"sensor\",\"timestamp\":");
    sprintf(numStr, "%lu", millis);
    strcat(target, numStr);
    strcat(target, ",\"temperature\": ");
    sprintf(numStr, "%.2f", *lastValue);
    strcat(target, numStr);
    strcat(target, ",\"sensor\":");
    sprintf(numStr, "%d", sensorId);
    strcat(target, numStr);
    strcat(target, "}");

    printf("Valeur de target: %s\n", target);
    fflush(stdout);

    return target;
}