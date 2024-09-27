#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <stdbool.h>

#define SIZE 1024

/*
 * Degree Celsius temperature value simulator.
 * As of today, it is the same algorithm than the one used for Temperature but it could be changed  if needed.
*/
void generate_temp_value(double *lastValue)
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

/**
 * Bar pressure value simulator.
 */
void generate_pressure_value(double *lastValue)
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
char *generate_json_doc(unsigned long millis, double *lastValue, int sensorId, bool isTemp)
{
    char *target = (char *)malloc(SIZE * sizeof(char));

    char unit[16];
    if(isTemp){
        generate_temp_value(lastValue);
        strcpy(unit, "\"° Celsius\"");
    }
    else {
        generate_pressure_value(lastValue);
        strcpy(unit, "\"Bar\"");
    }

    char numStr[20];
    strcpy(target, "{\"type\":\"sensor\",\"timestamp\":");
    sprintf(numStr, "%lu", millis);
    strcat(target, numStr);
    strcat(target, ",\"value\": ");
    sprintf(numStr, "%.2f", *lastValue);
    strcat(target, numStr);
    strcat(target, ",\"sensor\":");
    sprintf(numStr, "%d", sensorId);
    strcat(target, numStr);
    strcat(target, ",\"unit\":");
    strcat(target, unit);
    strcat(target, "}");

    printf("Target value: %s\n", target);
    fflush(stdout);

    return target;
}
