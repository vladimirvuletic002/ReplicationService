#include "common.h"

void printMeasurement(Measurement* m) {
    char time_string[50];
    strftime(time_string, sizeof(time_string), "%Y-%m-%d %H:%M:%S", &m->timeStamp);
    printf("ID uredjaja: %d\n", m->deviceId);
    if (m->type == CURRENT) {
        printf("Jacina struje: %.2f [A]\n", m->value);
    }

    else if (m->type == VOLTAGE) {
        printf("Napon: %.2f [V]\n", m->value);
    }

    else {
        printf("Snaga: %.2f [kW]\n", m->value);
    }

    printf("Vreme: %s\n", time_string);
}