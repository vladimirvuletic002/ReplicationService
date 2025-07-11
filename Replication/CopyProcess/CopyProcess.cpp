#include "CPProcess.h"
#define PROCESS_SERVER_IP "127.0.0.3"
#define MAX_MEASUREMENTS 100
std::atomic<bool> stopFlag = false;
int processID=-5;
bool info = false;   ///Don't print for faster transmission
///////////////////////////////////////////////////////////////////////
queue messageBackup;
std::mutex queueMutex;
int localCount = 0;

void inputCommands();

DWORD WINAPI getMessagesThread(LPVOID lpParam) {
   
    SOCKET getMessages = Connector(PROCESS_SERVER_IP, 9001);
    int empty=0;
    int iResult;
    Measurement m;

    while (!stopFlag) {

        m.purpose = 0;

        printf("\nWaiting for copy service to initiate backup\n");
        int bytesReceived = recv(getMessages, (char*)&m, sizeof(Measurement), 0);

        if (bytesReceived > 0) {
            printf("\n\Backup request received from process issued for device ID: %d\n", m.deviceId);
            if (processID == m.deviceId) {

                std::unique_lock<std::mutex> lock(queueMutex);
                if (!is_queue_empty(&messageBackup)) {

                    printf("\n\BACKUP request received\n TOTAL MESSAGES IN QUEUE: %d \n", messageBackup.size);
                    int i = 0;
                    while (!is_queue_empty(&messageBackup)) {
                        i++;
                        dequeue(&messageBackup, &m);
                        m.purpose = 505;
                        if (i < 100) {
                            printf("\n\t\t\t\tSending data...%d", i);
                        }
                        else if(i==100){
                            printf("\n\t\t\t\tWait for completion...");
                        }
                        send(getMessages, (const char*)&m, sizeof(Measurement), 0);

                    }
                    printf("\n\t\t\t\tDone.\tTotal: %d\n\n",i);
                    Measurement endSignal = {};
                    endSignal.deviceId = m.deviceId;
                    endSignal.purpose = END_OF_QUEUE;
                    send(getMessages, (const char*)&endSignal, sizeof(Measurement), 0);
                }
                else {
                    m.purpose = EMPTY_QUEUE;
                    send(getMessages, (const char*)&m, sizeof(Measurement), 0);
                }

            }
            else {

                m.purpose = SKIP_QUEUE;
                printf("\nSkipping backup...\n");
                send(getMessages, (const char*)&m, sizeof(Measurement), 0);

            }


        }
        else {
            printf("\n\n\n\t\t\t\tCopy service crashed...Shutting down in 3s...\n");
            Sleep(2500);
            stopFlag = true;
            break;
        }
            
    }
       
    
    if(getMessages!=INVALID_SOCKET){
    closesocket(getMessages);
    }
    return 1;
}


bool processDataFromCopy(queue &messageBackup, Measurement data, int bytesReceived, int &messNum)
{   

    if (bytesReceived <= 0) {
        printf("\n\--------NIJE STIGLO ALOOOOOO!!!:---------\n");
        return false;
    }
    else{
        if (info) {
            printf("\n\--------Data received from copy server!!!:---------\n");
            printMeasurement(&data);
        }
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            if (is_queue_empty(&messageBackup)) {
                
                processID = data.deviceId;
                printf("\nPROCESS ASSIGNED TO ID : %d", processID);
            }
            if (data.deviceId == processID) {
               
                if (enqueue(&messageBackup, data)) {
                    messNum++;
                    if (data.purpose==8 && !info) {
                        if(messNum%10000==0)
                        printf("\n\t\t\t\tAdded to queue...  %d", messNum);
                    }
                    else{
                       printf("\n\t\t\t\tAdded to queue.... %d : ID %d ",messNum,data.deviceId);
                    }
                }
                else {
                    printf("\n\t\t\t\tFailed to enqueue...%d", messNum);
                }
            }
        }
        return true;
    }


}




int start() {
   
    SOCKET connectSocket = INVALID_SOCKET;
    Measurement measurement;
    int iResult;
    int choice;
    int messNum = 0;
    u_short port = 6000;

    if (!InitializeWindowsSockets()) {
        printf("FAILED");
        return -1;
    }

    connectSocket = Connector(PROCESS_SERVER_IP, port);

    init_queue(&messageBackup);
    if (connectSocket == INVALID_SOCKET) {
        return -1;
    }
    Measurement data;

    HANDLE backupThread = CreateThread(nullptr, 0, getMessagesThread, &messageBackup, 0, nullptr);
    makeSocketNonBlocking(connectSocket);
    printf("\Waiting for measurements...\n");
    //inputCommands();
    while (!stopFlag) {

        int bytesReceived = recv(connectSocket, (char*)&data, sizeof(Measurement), 0);
        if (bytesReceived < 0) {
            if (stopFlag) {
                closesocket(connectSocket);
                break;
            }
            if (WSAGetLastError() == WSAEWOULDBLOCK) {
                // No connection attempt; continue the loop
                // Avoid busy-waiting
                Sleep(100);
                continue;
            }
            else {
                std::cerr << "\nFAILED: " << WSAGetLastError() << '\n';
                break;
            }
        }
        else {
         
            if (!processDataFromCopy(messageBackup, data, bytesReceived, messNum)) {
                printf("\nMessage receive faled!\n");
                break;
            }
        }

    }


    stopFlag = true;
    Sleep(100);

    
   
    // cleanup
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        if (!is_queue_empty(&messageBackup)) {
            free_queue(&messageBackup);
        }
    }
    if (connectSocket!=INVALID_SOCKET) {
        closesocket(connectSocket);
    }
    if (backupThread != NULL && backupThread != INVALID_HANDLE_VALUE) {
        CloseHandle(backupThread);
    }
    WSACleanup();

    return 0;
}

void inputCommands() {
    char ch;
    while (true) {
        ch = std::getchar();
        printf("\n\t\t\t\t(1) < --Exit\n\t\t\t\t(2) <-- Print all info\n\t\t\t\t");

        // std::cin >> ch;

        if (ch == '1')
        {
            printf("\n\nExiting\n");
            break;

        }
        else if (ch == '2')
        {
            if (info == true) {
                printf("\n\n\n\t\t\t\t--------------------------\n\t\t\t\tMore feedback turned OFF\n\t\t\t\t--------------------------\n");
                info = false;
            }
            else {

                info = true;
                printf("\n\n\n\t\t\t\t--------------------------\n\t\t\t\tMore feedback turned ON\n\t\t\t\t--------------------------\n");
            }


        }
        else {
            continue;
        }

    }
}

int main() {
    //startProcess(messageBackup, &queueMutex, processID);
    start();
    return 0;// start();
}
