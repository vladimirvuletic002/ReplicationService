#include<mutex>
#include "../Common/Queue.h"
#include "../Common/req_queue.h"
#include <iostream>
#define PROC_SERVER_IP_ADDRESS "127.0.0.1"
#define SERVER_IP_ADDRESS "127.0.0.2"
#define DEFAULT_PORT "8081"

//request_queue
req_queue socketQueue;

std::mutex messageMutex;
std::mutex socketMutex;
int currentProcessId = -1;
queue temporaryQueue;
std::condition_variable dataCondition;
std::atomic<bool> stopFlag = false;
std::atomic<int> counter = 0;
std::atomic<int> confirmedMess = 0;


std::atomic<bool> info = false;   ///Don't print for faster transmission
///////////////////////////////////////////////////////////////////////
/*DWORD WINAPI handleClients(LPVOID lparm) {
    bool serverConn;
    SOCKET socket;
    SOCKET serviceSocket;
    //SOCKET socket = *(SOCKET*)lparm;
    Measurement data;
    queue* messageQueue = (queue*)lparm;
    int iResult;
   
    while (true) {
        {
            iResult = 0;
            std::unique_lock<std::mutex> lock(socketMutex);
            dataCondition.wait(lock, [] { return !is_req_queue_empty(&socketQueue) || stopFlag==true; });
            
            if (stopFlag == false) {
                serviceSocket = Connector(SERVER_IP_ADDRESS, 8000);
                serverConn = true;
                socket = dequeue_request(&socketQueue);
            }
            else {
                printf("Server stopped");
                return 1;
            }
        }
        printf("\nClient connected.");
       
        while (1) {
             int bytesReceived = recv(socket, (char*)&data, sizeof(Measurement), 0);
            
            if (bytesReceived <= 0) {
                printf("\n-------------Client left...\n");
                break;
            }
                if (serverConn)
                    iResult = send(serviceSocket, (const char*)&data, sizeof(Measurement), 0);

                if (iResult == SOCKET_ERROR)
                {
                    printf("Send failed with error: %d\n\n Connection with server lost... [ Still have client, saving data to Temporary queue... ]\n", WSAGetLastError());
                    closesocket(serviceSocket);
                    serverConn = false;



                    continue;

                }
                if (data.purpose == 99) { // backup

                    std::atomic<bool> stop = false;
                    SOCKET connect = Connector(SERVER_IP_ADDRESS, 8500);
                    makeSocketNonBlocking(connect);
                    while (!stop) {
                        int non = nonBlockingReceive(stopFlag, connect, data, stop, info);
                        if (non!=SKIP && non!=END) {
                            if (info)
                                printf("\nBackup received from server \n");
                            printMeasurement(&data);
                            send(socket, (const char*)&data, sizeof(Measurement), 0);
                        }
                    }
                    printf("\nBackup for client done.");
                    data.purpose = END_OF_QUEUE;
                    send(socket, (const char*)&data, sizeof(Measurement), 0);
                    closesocket(connect);
                    continue;
                }
                if (data.purpose != 8) {
                    printf("\n\n---------Data received from device:---------\n");
                    printMeasurement(&data);
                }


                {
                    std::unique_lock<std::mutex> lock(messageMutex);
                    if (data.purpose != 99) {
                        enqueue(messageQueue, data); // Dodaj poruku u red za prosleđivanje kopiji
                        counter++;
                    }
                }
                confirmedMess++;
                if (info) {
                    std::cout << "\nID : " << data.deviceId << "\tMessages received : " << counter << "\tMessages successfully sent : " << confirmedMess;
                    //   free_deq(messageQueue); //message sent, can delete it
                }
                else {
                    if(counter%10000==0)
                    std::cout << "\nID : " << data.deviceId << "\tMessages received : " << counter << "\tMessages successfully sent : " << confirmedMess;

                }
            
            }
         
         }
        
     }*/

DWORD WINAPI handleClients(LPVOID lparm) {
    SOCKET socket;
    SOCKET serviceSocket = INVALID_SOCKET;
    Measurement data;
    queue* messageQueue = (queue*)lparm;
    bool serverConn = false;
    int iResult = 0;

    while (!stopFlag) {
        // Čekaj novi klijent
        {
            std::unique_lock<std::mutex> lock(socketMutex);
            dataCondition.wait(lock, [] { return !is_req_queue_empty(&socketQueue) || stopFlag == true; });

            if (stopFlag) {
                printf("Server stopped\n");
                return 1;
            }

            socket = dequeue_request(&socketQueue);
        }

        printf("\nClient connected.\n");

        // Gornja petlja šalje klijentu backup ako traži, sve ostale poruke šalje serveru ili u temporaryQueue
        while (!stopFlag) {
            // 1. UVEK POKUŠAJ RECONNECT NA SERVER AKO NIJE POVEZAN
            while (!serverConn && !stopFlag) {
                serviceSocket = Connector(SERVER_IP_ADDRESS, 8000);
                if (serviceSocket != INVALID_SOCKET) {
                    printf("[RECONNECT] Connected to server!\n");

                    // POKUŠAJ DA POŠALJEŠ SVE IZ temporaryQueue
                    while (!is_queue_empty(&temporaryQueue)) {
                        Measurement tempMsg;
                        {
                            std::unique_lock<std::mutex> lock(messageMutex);
                            dequeue(&temporaryQueue, &tempMsg);
                        }
                        int tempSend = send(serviceSocket, (const char*)&tempMsg, sizeof(Measurement), 0);
                        if (tempSend == SOCKET_ERROR) {
                            printf("Retry from tempQueue FAILED, returning message back and retrying connection...\n");
                            std::unique_lock<std::mutex> lock(messageMutex);
                            enqueue(&temporaryQueue, tempMsg); // vrati nazad
                            closesocket(serviceSocket);
                            serviceSocket = INVALID_SOCKET;
                            break; // Vrati se na reconnect petlju
                        }
                    }
                    if (is_queue_empty(&temporaryQueue)) {
                        serverConn = true; // Sva poruke iz reda su poslate!
                    }
                }
                else {
                    printf("[RECONNECT] Server unavailable, retrying in 1s...\n");
                    Sleep(1000);
                }
            }
            if (stopFlag) break;

            // 2. PRIMI NOVU PORUKU OD KLIJENTA (ili client disconnect)
            int bytesReceived = recv(socket, (char*)&data, sizeof(Measurement), 0);
            if (bytesReceived <= 0) {
                printf("\n-------------Client left...\n");
                break;
            }

            // 3. SPECIJALAN SLUČAJ: BACKUP ZAHTEV
            if (data.purpose == 99) {
                std::atomic<bool> stop = false;
                SOCKET connect = Connector(SERVER_IP_ADDRESS, 8500);
                makeSocketNonBlocking(connect);
                while (!stop) {
                    int non = nonBlockingReceive(stopFlag, connect, data, stop, info);
                    if (non != SKIP && non != END) {
                        if (info)
                            printf("\nBackup received from server \n");
                        printMeasurement(&data);
                        send(socket, (const char*)&data, sizeof(Measurement), 0);
                    }
                }
                printf("\nBackup for client done.");
                data.purpose = END_OF_QUEUE;
                send(socket, (const char*)&data, sizeof(Measurement), 0);
                closesocket(connect);
                continue;
            }

            // 4. POKUŠAJ DA POŠALJEŠ SERVERU!
            if (serverConn) {
                iResult = send(serviceSocket, (const char*)&data, sizeof(Measurement), 0);
                if (iResult == SOCKET_ERROR) {
                    printf("Send failed! Saving message to temporaryQueue and disconnecting.\n");
                    serverConn = false;
                    closesocket(serviceSocket);
                    serviceSocket = INVALID_SOCKET;

                    // ČUVAJ U temporaryQueue!
                    std::unique_lock<std::mutex> lock(messageMutex);
                    enqueue(&temporaryQueue, data);
                }
            }
            else {
                // Server nije konektovan, ČUVAJ U temporaryQueue!
                std::unique_lock<std::mutex> lock(messageMutex);
                enqueue(&temporaryQueue, data);
            }

            // STATISTIKA I ISPISI:
            if (data.purpose != 8) {
                printf("\n\n---------Data received from device:---------\n");
                printMeasurement(&data);
            }
            {
                std::unique_lock<std::mutex> lock(messageMutex);
                if (data.purpose != 99) {
                    enqueue(messageQueue, data);
                    counter++;
                }
            }
            confirmedMess++;
            if (info) {
                std::cout << "\nID : " << data.deviceId << "\tMessages received : " << counter << "\tMessages successfully sent : " << confirmedMess;
            }
        }

        // Na kraju (client disconnect): očisti socket
        closesocket(socket);
    }
    return 0;
}


//Cant properly turn of cuz of BLOCKING
DWORD WINAPI acceptorThread(LPVOID lpParam) {

    SOCKET serverSocket = *(SOCKET*)lpParam;

    while (!stopFlag) {
        SOCKET clientSocket = InitializeAcceptor(stopFlag, serverSocket);
        if (clientSocket != INVALID_SOCKET) {
            // Add the client socket to the queue
            {
                std::lock_guard<std::mutex> lock(socketMutex);
                enqueue_request(&socketQueue, clientSocket);
            }
            dataCondition.notify_one();  // Notify one thread in the thread pool
        }
        else {
            //printf("\n\nServer on port already running... shutting down...\n");
            //break;
            int errCode = WSAGetLastError();
            if (errCode == WSAEWOULDBLOCK) {
                // Nema konekcije, pokušaj opet za 100ms
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
            else {
                printf("\n\nServer accept error: %d\n", errCode);
                std::this_thread::sleep_for(std::chrono::seconds(2));
                continue;
            }
        }
    }
    return 0;
}


void process(const char*& port) {
    
    printf("\nSelect interface:\n");
    printf("1 --- Ampermeter\n");
    printf("2 --- Voltmeter\n");
    printf("3 --- Wattmeter\n");

    int bindToInterface = 0;
    if (scanf_s("%d", &bindToInterface) != 1) { // Check if input is valid
        printf("Invalid input. Defaulting to interface 1.\n");
        while (getchar() != '\n'); // Clear the input buffer
        bindToInterface = 1; // Default value

    }
    else {
        getchar();
    }
    
    if (bindToInterface == 1) {
        currentProcessId = 1;
        port = "8081";
    }
    else if (bindToInterface == 2) {
        currentProcessId = 2;
        port = "8082";
    }
    else if (bindToInterface == 3) {
        currentProcessId = 3;
        port = "8083";
    }
    else {
       
        port = "8081";
    }
}


void InputCommands() {
    char ch;
    while (true) {

        printf("\n\n(1) < --Exit\n(2) <-- Print all info\nTotal messages passed through: %d\n\nprocess::id:%d>", counter.load(),currentProcessId);
        ch = std::getchar();
        // std::cin >> ch;

        if (ch == '1')
        {
            printf("\n\nExiting\n");
            break;

        }
        else if (ch == '2')
        {
            if (info == true) {
                printf("\n\t\t\t\t--------------------------\n\t\t\t\tMore feedback turned OFF\n\t\t\t\t--------------------------\n");
                info = false;
            }
            else {

                info = true;
                printf("\n\t\t\t\t--------------------------\n\t\t\t\tMore feedback turned ON\n\t\t\t\t--------------------------\n");
            }

        }
        else {
            continue;
        }

    }

}

int main() {
    
    queue messageQueue;
    init_queue(&messageQueue);

    init_req_queue(&socketQueue, 100);
    
    const char* port = "";
    
    process(port);
    SOCKET serverSocket = INVALID_SOCKET;
    SOCKET acceptedSocket = INVALID_SOCKET;
    int iResult;
    InitializeWindowsSockets();

    serverSocket= initializeListener(PROC_SERVER_IP_ADDRESS, port);
    printf("Server initialized, waiting for clients.\n");
    
    // Create thread pool
    const int THREAD_POOL_SIZE = 4;
    HANDLE workerThreads[THREAD_POOL_SIZE];
    HANDLE acceptor = CreateThread(nullptr, 0, acceptorThread, &serverSocket, 0, nullptr);

    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        workerThreads[i] = CreateThread(nullptr, 0, handleClients, &messageQueue, 0, nullptr);
    }

    std::cout << "\nPress Enter for Menu..." << std::endl;
    InputCommands();
   
    // Signal threads to stop
    {
        std::lock_guard<std::mutex> lock(socketMutex);
        stopFlag = true;
    }
    dataCondition.notify_all();
    Sleep(100);
  //  print_queue(&messageQueue);
    if (is_queue_empty(&messageQueue)) {
        free(&messageQueue);
    }

    // Wait for threads to exit
    WaitForMultipleObjects(THREAD_POOL_SIZE, workerThreads, TRUE, INFINITE);
    WaitForSingleObject(acceptor, INFINITE);

    // Cleanup
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        CloseHandle(workerThreads[i]);
    }

    CloseHandle(acceptor);


    while (!is_req_queue_empty(&socketQueue)) {
        SOCKET sock = dequeue_request(&socketQueue);
        if (sock != INVALID_SOCKET) {
            closesocket(sock);
        }
    }

    free_req_queue(&socketQueue);
    

    closesocket(serverSocket);
    WSACleanup();
    
    //std::cout << "Server shut down." << std::endl;
    return 0;




}

