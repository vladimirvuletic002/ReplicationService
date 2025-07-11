#ifndef COMMON_H
#define COMMON_H

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN


#include <mutex>
#include <condition_variable>
#include <string>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <iostream>

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
    //struct tm timeStamp;
    int64_t epochTime;
} Measurement;

typedef struct node {
    Measurement data;
    struct node* next;
} node;

typedef struct {
    node* head;  //Pok. na prvi el. reda
    node* tail;  //Pok. na poslednji el. reda
    int size;   //broj el. u redu
} queue;


typedef struct KeyValuePair {
    int key;                // Kljuc je tipa `int`
    queue value;            // Vrednost je tipa `queue`
    struct KeyValuePair* next; // Za ulancavanje
} KeyValuePair;

typedef struct {
    KeyValuePair** buckets; // Niz pokazivaca na ulancane liste
    size_t capacity;        // Kapacitet tabele
    size_t size;            // Trenutni broj elemenata
} unordered_map;

typedef struct KeyValuePairMtx {
    int key;                // Kljuc je tipa `int`
    std::mutex value;            // Vrednost je tipa `mutex`
    struct KeyValuePairMtx* next; // Za ulancavanje
} KeyValuePairMtx;

typedef struct {
    KeyValuePairMtx** buckets; // Niz pokazivaca na ulancane liste
    size_t capacity;        // Kapacitet tabele
    size_t size;            // Trenutni broj elemenata
} unordered_mtx_map;

typedef struct KeyValuePairCondVar {
	int key;
	std::condition_variable value;
	struct KeyValuePairCondVar* next;
} KeyValuePairCondVar;

typedef struct {
	KeyValuePairCondVar** buckets; // Niz pokazivaca na ulancane liste
	size_t capacity;        // Kapacitet tabele
	size_t size;            // Trenutni broj elemenata
} unordered_condVar_map;


SOCKET Connector(const char* SERVICE_IP_ADDRESS, u_short port);
SOCKET initializeListener(const char* ip_address, const char* port);
SOCKET InitializeAcceptor(std::atomic<bool>& stopFlag, SOCKET serverSocket);
bool InitializeWindowsSockets();
void makeSocketNonBlocking(SOCKET socket);
bool SendMessageTo(SOCKET socket, Measurement data);
bool EnqueueToMap(std::mutex& queueMtx, unordered_map &messageQueues, Measurement data);
int ReceiveWithNonBlockingSocket(std::atomic<bool>& stopFlag, SOCKET& socket, Measurement& m);
int nonBlockingReceive(std::atomic<bool>& stopFlag, SOCKET& socket, Measurement& data, std::atomic<bool>& stop,bool print);
bool SafeEnqueue(queue *queue, std::mutex& qMtx, Measurement m);
bool SafeDequeue(queue *queue, std::mutex& qMtx, Measurement* m);
void printQueue(queue& q, std::mutex& mtx);
void printQueueSelection(unordered_map& messageQueues, unordered_mtx_map &queueMtxs, unordered_map& messageQueuesBackup, unordered_mtx_map queueMtxsBackup, char c);
bool printer(unordered_map& messageQueues, unordered_mtx_map &queueMtxs);
void printMeasurement(Measurement* m);

#endif
