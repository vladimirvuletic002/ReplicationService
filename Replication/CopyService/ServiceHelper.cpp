#include<mutex>
#include<unordered_map>
#include "../Common/Common.h"
#include <iostream>
#include <process.h>

#define SERVICE_IP_ADDRESS "127.0.0.3"
#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "8080"

#define EMPTY_QUEUE -3
#define END_OF_QUEUE -1
#define SKIP_QUEUE -2


bool ProcessDataTransfer33(SOCKET& clientSocket, SOCKET processSocket, Measurement& m, int currentSocket, std::atomic<bool>& info,bool& stopFlag) {
    int bytesReceived = 0;
    bool data = false;
    while (true) {
        m.purpose = 0;
        bytesReceived = recv(processSocket, (char*)&m, sizeof(Measurement), 0);
        if (bytesReceived > 0) {

            if (m.purpose == SKIP_QUEUE) {

                break;

            } if (m.purpose == EMPTY_QUEUE) {

                send(clientSocket, (const char*)&m, sizeof(Measurement), 0);
                data = true;

                break;

            }
                if(info==true)
                printf("\n\nBackup REQUEST received from process, sending to original service %d :---------\n");
                printMeasurement(&m);
                
                int res=send(clientSocket, (const char*)&m, sizeof(Measurement), 0);
                if (res <= 0) { stopFlag = true; }
            if (m.purpose == END_OF_QUEUE) {

                printf("\nData transfer stopped");
                data = true;
                break;

            }
        }
        else {
            stopFlag = true;
            break;
        }

    }
    return data;
}


void HandleBackupRequest22(SOCKET& clientSocket, SOCKET processSockets[3], Measurement& m, Measurement& measure, std::atomic<bool>& info,bool& stopFlag) {
    int num = 0;

    for (int i = 0; i < 3; i++) {

      //  printf("looping %d time", i + 1);
        send(processSockets[i], (char*)&measure, sizeof(Measurement), 0);
        if (ProcessDataTransfer33(clientSocket, processSockets[i], m, i,info,stopFlag)) {
            ++num;
        }
        //printf("\nGOING BACK MATE");

        if (i == 2 && num < 1) {
            printf("\nServer requested for measurements for device ID: %d , no process with such queue... Stopping backup... ",m.deviceId);
            m.purpose = -4;
            send(clientSocket, (const char*)&m, sizeof(Measurement), 0);
        }
        m.purpose = 0;
    }
}

std::wstring stringToWString(const std::string& str) {
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstrTo[0], size_needed);
    return wstrTo;
}

// Function to start a process in the current directory
void startProcessInCurrentDirectory(const std::wstring& processName) {
    // Get the current executable's directory
    wchar_t currentDir[MAX_PATH];
    if (GetModuleFileNameW(NULL, currentDir, MAX_PATH)) {
        // Extract the directory path
        std::wstring directory(currentDir);
        directory = directory.substr(0, directory.find_last_of(L"\\/"));

        // Full path to the target process
        std::wstring processPath = directory + L"\\" + processName;

        // Set up process startup information and process information structures
        STARTUPINFOW si = { sizeof(STARTUPINFOW) };
        PROCESS_INFORMATION pi = {};

        // Create the process
        if (CreateProcessW(
            processPath.c_str(),  // Path to the executable
            NULL,                 // Command line arguments
            NULL,                 // Process handle not inheritable
            NULL,                 // Thread handle not inheritable
            FALSE,                // No inheritance of handles
            CREATE_NEW_CONSOLE,                    // No creation flags
            NULL,                 // Use parent's environment block
            directory.c_str(),    // Set the working directory
            &si,                  // Pointer to STARTUPINFO
            &pi                   // Pointer to PROCESS_INFORMATION
        )) {
          //  std::wcout << L"Process started successfully: " << processPath << std::endl;

            // Close handles to the process and its primary thread
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
        else {
            std::wcerr << L"Failed to start process: " << GetLastError() << std::endl;
        
        }
    }
    else {
        std::wcerr << L"Failed to get current directory: " << GetLastError() << std::endl;
    }
  
}
