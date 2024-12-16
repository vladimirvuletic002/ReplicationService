#include "ServiceHelper.h"
#define COPY_SERVER_IP_ADDRESS "127.0.0.3"
#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "8081"



req_queue socketQueue;
req_queue socketProcessQueue;

std::mutex socketMtxQueue;
std::mutex messageMutex;
std::mutex socketMutex;

std::condition_variable serviceConnection;
std::condition_variable processConnection;

std::unordered_map<int, queue> messageQueues;
std::unordered_map<int, std::mutex> queueMtxs;
std::unordered_map<int, std::condition_variable> dataCondition;

std::atomic<bool> info = false;

struct parameters {
    SOCKET serverSocket;
    int ThreadId;
};

std::atomic<bool> stopFlag = false;
std::atomic<int> counter = 0;



DWORD WINAPI EnqueueAndNotifyThread(LPVOID lparm) {

    SOCKET socket=INVALID_SOCKET;
    SOCKET serviceSocket=INVALID_SOCKET;
    //SOCKET socket = *(SOCKET*)lparm;
    Measurement data;
    queue* messageQueue = (queue*)lparm;
    //serviceSocket = connectToProcess();
    while (!stopFlag) {

        {
            std::unique_lock<std::mutex> lock(socketMutex);
            serviceConnection.wait(lock, [] { return !is_req_queue_empty(&socketQueue) || stopFlag == true; });
            
            if (stopFlag == false) {
                socket = dequeue_request(&socketQueue);
            }
            else {
                printf("\n\t\t\t\tThread finished.");
              
                if (serviceSocket)
                    closesocket(serviceSocket);
                return 1;
            }
        }

        while (!stopFlag) {

            int bytesReceived = recv(socket, (char*)&data, sizeof(Measurement), 0);
            
            if (bytesReceived <= 0) {
                closesocket(socket);
                break;
            }
            else if (bytesReceived > 0) {

                if (EnqueueToMap(queueMtxs[data.deviceId], messageQueues, data)) {
                    dataCondition[data.deviceId].notify_all();

                }
                
            }
        }
    }

}

DWORD WINAPI getDataFromQueueThread(LPVOID lParam) {
    int ProcessId = *(int*)lParam;
    SOCKET socket;
    startProcessInCurrentDirectory(L"CopyProcess.exe");
    printf("\nThread active for process ID = %d", ProcessId);

    while (true) {

        {
            std::unique_lock<std::mutex> lock(socketMtxQueue);
            processConnection.wait(lock, [] { return !is_req_queue_empty(&socketProcessQueue) || stopFlag == true; });

            if (stopFlag == false) {
                socket = dequeue_request(&socketProcessQueue);
            }
            else {
                printf("\n\t\t\t\tThread stopped.");
                return 1;
            }
        }   
    //    printf("\nWaiting for data on thread for process ID = %d ", ProcessId);
            while (true) {
                {
                    std::unique_lock<std::mutex> lock(queueMtxs[ProcessId]);
                    dataCondition[ProcessId].wait(lock, [ProcessId] { return !is_queue_empty(&messageQueues[ProcessId]) || stopFlag == true; });
                }
                    if (stopFlag == false) {

                        {
                         // std::unique_lock<std::mutex> lock(queueMtxs[ProcessId]);
                            Measurement m;
                            SafeDequeue(&messageQueues[ProcessId],queueMtxs[ProcessId],&m);

                            if (send(socket, (const char*)&m, sizeof(Measurement), 0)>0) {
                               
                                    counter++;
                                    if (counter % 10000 == 0) {
                                        printf("\n\t\t\t\tMessage sent to process ID:%d :::%d",m.deviceId, counter.load());
                                    }
                                    if (info)
                                    printf("\n\t\t\t\tSent to process ID - %d\t Message counter: %d", ProcessId, counter.load());
                                
                            }
                            else {
                                printf("\nFailed to send, message going back to temporary queue");
                                SafeEnqueue(&messageQueues[ProcessId],queueMtxs[ProcessId],m);
                            }

                        }
                    }
                    else {
                        printf("\n\t\t\t\tThread stopped.");
                        return 1;
                    }
                }
                
    }


}





void makeSocketNonBlocking(SOCKET socket) {
    u_long mode = 1; // 1 to enable non-blocking mode
    ioctlsocket(socket, FIONBIO, &mode);
}



DWORD WINAPI getMessagesThread(LPVOID lpParam) {
    
    SOCKET getMessages=initializeListener(COPY_SERVER_IP_ADDRESS, "9000");
    SOCKET processMessages = initializeListener(COPY_SERVER_IP_ADDRESS, "9001");
    SOCKET processSockets[3];
    int numOfRetryAttempts = 0;
    sockaddr_in processAddr;
    makeSocketNonBlocking(getMessages);

    for (int i = 0; i < 3; i++) {

        processSockets[i] = accept(processMessages, (sockaddr*)&processAddr, nullptr);

        if (processSockets[i] == INVALID_SOCKET) {
            if (stopFlag) break;  // Stop if flagged for shutdown
            std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
            continue;
        }
        printf("\n\t\t\tProcess %d / 3 connected", i);
    }
    printf("\n\t\t\tAll 3 processes connected, ready to backup\n");
    
    printf("\n\t\t\t\tWaiting for original server...\n");
    while (!stopFlag) {

        sockaddr_in clientAddr;
        int clientAddrLen = sizeof(clientAddr);
        SOCKET clientSocket = accept(getMessages, (sockaddr*)&clientAddr, &clientAddrLen);
        if (clientSocket == INVALID_SOCKET) {

            if (stopFlag) {
                printf("\nOriginal server shut down, closing connection...\n");
                closesocket(clientSocket);
                break;
            }
            if (WSAGetLastError() == WSAEWOULDBLOCK) {
                Sleep(150);
                continue;
            }
            else {
                std::cerr << "\nAccept failed: " << WSAGetLastError() << '\n';
                break;
            }
        }
        int num = 0;
        bool stop = false;
        int iResult;
        Measurement m;
        Measurement measure;
        printf("\nServer connected to request backup!\n");
        while (!stop && !stopFlag) {
            // makeSocketNonBlocking();
         
          //  printf("\n\t\t\t\tWaiting for backup request from service...\n");
            u_long mode = 0; // 0 for blocking mode
            ioctlsocket(clientSocket, FIONBIO, &mode);

            int bytesReceived = recv(clientSocket, (char*)&measure, sizeof(Measurement), 0);
            if (bytesReceived > 0) {
                printf("\nReceived data request, searching for process with requested queue ID...");
                makeSocketNonBlocking(clientSocket);
              switch (ReceiveWithNonBlockingSocket(stopFlag, clientSocket, m)) {
                case END:
                    printf("\n\t\t\t\tOriginal server shut down mid backup, closing everything...\n");
                    Sleep(1000);
                    stopFlag = true;
                    break;

                case SKIP:

                case PROCEED:
                    mode = 0; // 0 for blocking mode
                    ioctlsocket(clientSocket, FIONBIO, &mode);
                    HandleBackupRequest22(clientSocket, processSockets, m, measure,info,stop);
                    break;        //
               
                default:
                    break;

              }
            }
            else {
                std::cerr << WSAGetLastError() << '\n';
                printf("\n\t\t\t\tDone. Connection with server backup thread closed.\n\t\t\t\tReady for another backup...\n");
                closesocket(clientSocket);
                break;
              
            }

        }
    }

    printf("\n\t\t\t\tClosing all sockets...\n");
    for (int i = 0; i < 3; i++) {
        closesocket(processSockets[i]);
    }
    if(processMessages!=INVALID_SOCKET)
    closesocket(processMessages);

    if(processMessages!=INVALID_SOCKET)
    closesocket(getMessages);
    return 1;
 }


 void InputCommands() {
     char ch;
     while (true) {
         ch = std::getchar();
         printf("\n\t\t\t\t(1) < --Exit\n\t\t\t\t(2) <-- Print all info\n\t\t\t\tTotal messages passed through: %d\n\n\t\t\tcopy_service::>", counter.load());
         
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



void printSockaddr(const sockaddr_in& addr) {
    char ipStr[INET_ADDRSTRLEN]; // Buffer for IP address string

    // Convert IP address to human-readable string
    inet_ntop(AF_INET, &(addr.sin_addr), ipStr, INET_ADDRSTRLEN);

    // Convert port number from network byte order to host byte order
    unsigned short port = ntohs(addr.sin_port);

    std::cout <<"\nIP Address : " << ipStr ;
    std::cout << "\nPort: " << port;
}




DWORD WINAPI acceptorThread(LPVOID lpParam) {

    std::cout << GetCurrentThreadId() << std::endl;

    parameters* pars = (parameters*)lpParam;
    SOCKET serverSocket = pars->serverSocket;
    SOCKET clientSocket = INVALID_SOCKET;
    makeSocketNonBlocking(serverSocket);
    while (!stopFlag) {

        SOCKET clientSocket = InitializeAcceptor(stopFlag, serverSocket);
        if (stopFlag)break;
        if (clientSocket == INVALID_SOCKET)
        {
            int errCode = WSAGetLastError();
            if (errCode == WSAEWOULDBLOCK) {
                // Non-blocking socket: No incoming connection yet, continue
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
            else {
                // Fatal error: Break the loop
                closesocket(clientSocket);
                printf("\n\nError: %d. Shutting down...\n", errCode);
                break;
                }
          
        
       }else {
            
            if (pars->ThreadId == 0) {
                std::cout << "\nNew connection from Main server!" << std::endl;
                //printSockaddr(clientAddr);
                {
                    std::unique_lock<std::mutex> lock(socketMutex);
                    enqueue_request(&socketQueue, clientSocket);
                    serviceConnection.notify_one();
                }
            }
            else if (pars->ThreadId == 1) {
                {
                    std::cout << "\nNew Process connected!" << std::endl;
                    std::unique_lock<std::mutex> lock(socketMtxQueue);
                    enqueue_request(&socketProcessQueue, clientSocket);
                    processConnection.notify_all();
                }
            }

        }
    }
    if (clientSocket != INVALID_SOCKET)closesocket(clientSocket);
    
    
    return 1;
}

int main() {
   
    InitializeWindowsSockets();
    SOCKET processListener=initializeListener(COPY_SERVER_IP_ADDRESS, "6000");
    SOCKET serviceListener = initializeListener(COPY_SERVER_IP_ADDRESS, "7000");
    // Create thread pool
    const int THREAD_POOL_SIZE = 9;
    const int PROCESS_POOL_SIZE = 3;
    
    HANDLE workerThreads[THREAD_POOL_SIZE];
    HANDLE workerProcThreads[PROCESS_POOL_SIZE];

    
    struct parameters serviceP;
    serviceP.serverSocket = serviceListener;
    serviceP.ThreadId = 0;

    struct parameters processP;
    processP.serverSocket = processListener;
    processP.ThreadId = 1;

   
    HANDLE acceptor = CreateThread(nullptr, 0, acceptorThread,&serviceP, 0, nullptr);
    HANDLE acceptorProc = CreateThread(nullptr, 0, acceptorThread, &processP, 0, nullptr);
    HANDLE backupThread = CreateThread(nullptr, 0, getMessagesThread, nullptr, 0, nullptr);

    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
      
        workerThreads[i] = CreateThread(nullptr, 0, EnqueueAndNotifyThread, &messageQueues, 0, nullptr);
    }

    int threadId[3];
    for (int i = 0; i < PROCESS_POOL_SIZE;i++) {
        threadId[i] = i + 1;
        workerProcThreads[i] = CreateThread(nullptr, 0,getDataFromQueueThread, &threadId[i], 0, nullptr);
    }
    // Wait for user input to shut down the server
    InputCommands();
   

    // Signal threads to stop
    {
        std::lock_guard<std::mutex> lock(socketMutex);
        stopFlag = true;
    }
    serviceConnection.notify_all();
    processConnection.notify_all();
    
    // Wait for threads to exit
    WaitForMultipleObjects(THREAD_POOL_SIZE, workerThreads, TRUE, INFINITE);
    WaitForSingleObject(acceptor, INFINITE);

    // Cleanup
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        CloseHandle(workerThreads[i]);
       
    }
    for (int i = 0; i < PROCESS_POOL_SIZE; i++) {
      
        CloseHandle(workerProcThreads[i]);
    }
   
    CloseHandle(acceptor);
    CloseHandle(acceptorProc);
    CloseHandle(backupThread);
    closesocket(serviceListener);
    closesocket(processListener);
    WSACleanup();

    std::cout << "\n\n\n\t\t\t\tDone. Copy server shut down." << std::endl;
    Sleep(2000);
    //std::cout << "Server shut down." << std::endl;
    return 0;




}

