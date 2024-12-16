#pragma once
#include <mutex>
#include <queue>
#include <unordered_map>
//#include "../Common/Common.h"
#include "../Common/Common.h"
#include <iostream>
#define SERVER_IP_ADDRESS "127.0.0.2"
#define COPY_SERVER_IP_ADDRESS "127.0.0.3"
#define NUMBER_OF_POSSIBLE_IDS 3
#define END 1
#define SKIP 2
#define PROCEED 3

typedef struct Params {

	SOCKET serverSocket;
	int id;

}Params;
std::queue<SOCKET> socketQueue;

std::mutex messageMutex;
std::mutex socketMutex;
std::mutex backupMutex;

std::condition_variable dataCondition;
std::condition_variable backupCondition;

unordered_map messageQueues;
std::unordered_map<int, std::mutex> queueMtxs;



void printfP(const char* text, Measurement* m, std::atomic<int> totalMessages, std::atomic<int> totalBackuo);
void ReceiveAndFilterMessages(std::atomic<bool>& stopFlag, Measurement& m, SOCKET& backupRequest,std::atomic<int>& totalBackup,
bool& stop, int& localCounter, unordered_map &messageQueue, std::mutex& mtx);
//void ReceiveAndFilterMessages(std::atomic<bool>& stopFlag, Measurement& m, SOCKET& backupRequest, int& counter,bool& stop, int& localCounter, queue& backupTempQeueu);