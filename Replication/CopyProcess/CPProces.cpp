
#include "CPProcess.h"
#define SERVER_IP_ADDRESS "127.0.0.4"
#define MAX_MEASUREMENTS 100




bool processDataFromCopy(SOCKET connectSocket, Measurement data, int bytesReceived,
                          int& messNum, std::mutex *queueMutex, queue &messageBackup, int &processID)
{

    if (bytesReceived <= 0) {
        return false;
    }
    else {

        printf("\n\--------Data recieved from copy server:---------\n ID: %d\n Current=%.2f [A]\n Voltage=%.2f [V]\n Power=%.2f [kW]\n-----------------------------------------------\n",
            data.deviceId, data.current, data.voltage, data.power);

        {
            std::unique_lock<std::mutex> lock(*queueMutex);
            if (is_queue_empty(&messageBackup)) {

                processID = data.deviceId;
                printf("\nPROCESS ID : %d", processID);
            }
            if (data.deviceId == processID) {
                if (enqueue(&messageBackup, data)) {
                    messNum++;
                    printf("Added to queue...  %d", messNum);
                }
            }
        }
        return true;
    }


}

int startProcess(queue &messageBackup,std::mutex* queueMutex, int &processID) {
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
    //Connector("127.0.0.4", port, connectSocket);
    connectSocket = Connector(SERVER_IP_ADDRESS, port);
    init_queue(&messageBackup);

    Measurement data;

    while (true) {

        printf("\nWaiting for measurements...\n");
        int bytesReceived = recv(connectSocket, (char*)&data, sizeof(Measurement), 0);
        if (!processDataFromCopy(connectSocket, data, bytesReceived, messNum,queueMutex,messageBackup,processID)) {
            break;
        }
    }
    // cleanup

    closesocket(connectSocket);
    WSACleanup();

    return 0;
}

