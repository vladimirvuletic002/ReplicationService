#pragma once
//#include "../Common/Common.h"
#include "../Common/Common.h"
#include "../Common/req_queue.h"


bool ProcessDataTransfer33(SOCKET& clientSocket, SOCKET processSocket, Measurement& m, int currentSocket,std::atomic<bool>& info,bool& stopFlag);
void HandleBackupRequest22(SOCKET& clientSocket, SOCKET processSockets[
], Measurement& m, Measurement& measure, std::atomic<bool>& info,bool& stopFlag);
void startProcessInCurrentDirectory(const std::wstring& processName);
bool receiveFull(SOCKET socket, char* buffer, int totalBytes);