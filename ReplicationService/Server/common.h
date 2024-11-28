#ifndef COMMON_H
#define COMMON_H

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define DEFAULT_PORT "8080"

typedef enum {
    CURRENT,
    VOLTAGE,
    POWER
} quantity;

typedef struct Measurement {
    int deviceId;       // Redni broj uređaja
    quantity type;      // Velicina koje se meri(struja,napon,snaga)
    float value;        // Vrednost merenja([A], [V], [kW])
    struct tm timeStamp;
} Measurement;

void printMeasurement(Measurement* m);

#endif
