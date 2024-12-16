#pragma once

#ifndef COMMON_H

#define COMMON_H

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#define WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define DEFAULT_PORT "8080"


typedef struct Measurement {
    int deviceId;       // Redni broj uređaja
    float current;      // Jačina struje (A)
    float voltage;      // Napon (V)
    float power;        // Snaga (kW)
} Measurement;

#endif