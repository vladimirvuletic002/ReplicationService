#ifndef COMMON_H
#define COMMON_H

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN


#include <mutex>
#include <unordered_map>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "unordered_map.h"
#include "Queue.h"
#include "req_queue.h"

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define END 1
#define SKIP 2
#define PROCEED 3

#define EMPTY_QUEUE -3
#define END_OF_QUEUE -1
#define SKIP_QUEUE -2

typedef enum {
    CURRENT,
    VOLTAGE,
    POWER
} quantity;

typedef struct Measurement {
    int deviceId;       // Redni broj uredjaja
    quantity type;      // Velicina koje se meri(struja,napon,snaga)
    float value;        // Vrednost merenja([A], [V], [kW])
    int purpose = 0;
    struct tm timeStamp;
} Measurement;

Measurement merenie;

/*typedef struct node {
    Measurement data;
    struct node* next;
} node;

typedef struct queue {
    node* head;  //Pok. na prvi el. reda
    node* tail;  //Pok. na poslednji el. reda
    int size;   //broj el. u redu
} queue;*/


SOCKET Connector(const char* SERVICE_IP_ADDRESS, u_short port);
SOCKET initializeListener(const char* ip_address, const char* port);
SOCKET InitializeAcceptor(std::atomic<bool>& stopFlag, SOCKET serverSocket);
bool InitializeWindowsSockets();
bool SendMessageQ(SOCKET copyServiceSocket, Measurement data, queue *messageQueue, std::mutex &queueMtx);
bool SendMessageTo(SOCKET socket, Measurement data);
bool EnqueueToMap(std::mutex& queueMtx, unordered_map &messageQueues, Measurement data);
int ReceiveWithNonBlockingSocket(std::atomic<bool>& stopFlag, SOCKET& socket, Measurement& m);
int nonBlockingReceive(std::atomic<bool>& stopFlag, SOCKET& socket, Measurement& data, std::atomic<bool>& stop,bool print);
bool SafeEnqueue(queue *queue, std::mutex& qMtx, Measurement m);
bool SafeDequeue(queue *queue, std::mutex& qMtx, Measurement* m);
void printQueue(queue& q, std::mutex& mtx);
void printQueueSelection(unordered_map &messageQueues, std::unordered_map<int, std::mutex>& queueMtxs, unordered_map &messageQueuesBackup, std::unordered_map<int, std::mutex>& queueMtxsBackup, char c);
void printer(unordered_map &messageQueues, std::unordered_map<int, std::mutex>& queueMtxs);
void printMeasurement(Measurement* m);

#endif