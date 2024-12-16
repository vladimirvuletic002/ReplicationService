#pragma once

//#include "../Common/Common.h"
#include "../Common/Queue.h"
#include<iostream>
#include <mutex>

int startProcess(queue& messageBackup, std::mutex* queueMutex, int& processID);


bool processDataFromCopy(SOCKET connectSocket, Measurement data, int bytesReceived,
    int& messNum, std::mutex& queueMutex, queue& messageBackup, int& processID);