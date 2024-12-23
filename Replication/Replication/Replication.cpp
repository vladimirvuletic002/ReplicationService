#pragma once
#include"ReplicationHelper.h"
req_queue socketQueue;

std::mutex messageMutex;
std::mutex socketMutex;
std::mutex backupMutex;

std::condition_variable dataCondition;
std::condition_variable backupCondition;

unordered_map messageQueues;
unordered_mtx_map queueMtxs;

std::mutex messageCounter;
std::atomic<int> totalMessages = 0;
std::atomic<int> totalBackup = 0;

int currentBackupChoice;
bool clientInitiated = false;
bool automaticBackupFetch = false;
std::atomic<bool> stopFlag = false;
std::atomic<bool> backup = false;

std::atomic<bool> info = false;   ///Don't print for faster transmission
///////////////////////////////////////////////////////////////////////
std::mutex backupQueueMutex;



unordered_map messageQueuesBackup;
unordered_mtx_map queueMtxsBackup;
std::mutex backupCMutex;
std::condition_variable backupConditionClient;
req_queue backupConnections;




void makeSocketNonBlocking(SOCKET socket) {
    u_long mode = 1; // 1 to enable non-blocking mode
    ioctlsocket(socket, FIONBIO, &mode);
}


void InitializeQu(Measurement data,int &currentId) {
    
    if (data.deviceId > 0 && data.deviceId <= NUMBER_OF_POSSIBLE_IDS) {

        if (!contains_key(&messageQueues, data.deviceId)) {
            queue q;
            init_queue(&q);
            insert_queue_to_map(&messageQueues, data.deviceId, &q);
        }
        currentId = data.deviceId;
    }
}



DWORD WINAPI handleProcesses(LPVOID lparm) {
    SOCKET socket;
    SOCKET copyServiceSocket = INVALID_SOCKET;
    int currentId = -1;
    Measurement data;
    queue* messageQueue = (queue*)lparm;
    

    int TotalPerThread = 0;

    while (true) {

        {
            currentId = -1;
            std::unique_lock<std::mutex> lock(socketMutex);
            dataCondition.wait(lock, [] { return !is_req_queue_empty(&socketQueue) || stopFlag == true; });
           
            backup = false;
            

            if (stopFlag == false) {
                copyServiceSocket = Connector(COPY_SERVER_IP_ADDRESS, 7000);
                socket = dequeue_request(&socketQueue);
                
            }
            else {
                printf("\n\t\t\t\tThread finished.");
                if(copyServiceSocket)
                closesocket(copyServiceSocket);
                return 1;
            }
        }
        makeSocketNonBlocking(socket);
      //  printfP("Waiting for data...\t\t[\tPress enter for menu!\t]", nullptr,totalMessages.load(), totalBackup.load());
        while (true) {


            int bytesReceived = recv(socket, (char*)&data, sizeof(Measurement), 0);

            if (bytesReceived <= 0 || stopFlag) {

                if (stopFlag == true) {
                    closesocket(copyServiceSocket);
                    closesocket(socket);
                    printf("\n\n\t\t\t\tForce server shutdown! ID ---> %d\n\b", currentId);
                    break;

                }
                int error = WSAGetLastError();
                if (error == WSAEWOULDBLOCK) {
                    // No data available yet; wait and retry
                    std::this_thread::sleep_for(std::chrono::milliseconds(150));
                    continue;
                }
                else {
                  closesocket(copyServiceSocket);
                  closesocket(socket);
                    std::cerr << "\n\t\t\tConnection with process closed : Error: " << error << '\n';
                    break;
                }

 
            }           
            if (currentId == -1) {
                InitializeQu(data, currentId);
            }


            if (currentId == data.deviceId) {                   
                
                if (data.purpose == 99) {//Client initiated backup
                    printf("\nCLIENT INITIATED BACKUP");
                    currentBackupChoice = data.deviceId;
                    backup = true;
                    clientInitiated = true;
                    backupCondition.notify_one();
                    continue;
                }

                insert_mtx_to_map(&queueMtxs, currentBackupChoice);
                std::mutex* mtxForDevice = get_mtx_for_key(&queueMtxs, currentBackupChoice);

                if (mtxForDevice == NULL) {
                    std::cerr << "Mutex for key " << currentBackupChoice << " not found!\n";
                    return -1;
                }

                if (SendMessageQ(copyServiceSocket,data,get_queue_for_key(&messageQueues, data.deviceId), *mtxForDevice)) {

                            
                           
                            if (data.purpose != 8) {
                                printfP("New manually sent data", &data,totalMessages.load(), totalBackup.load());
                            }
                            {                               
                                std::unique_lock<std::mutex> lock(messageCounter);
                                totalMessages++;
                                if (totalMessages % 1000==0)
                                    printf("\n\t\t\t\tTotal messages received = %d\n\t\t\t\tSuccessfully forwarded to copy server.\n", totalMessages.load());
   
                            }
                }
                    else {
                        printf("\nFailed to send to copy service.Trying to connect again!");//////////////////////////////////////////////////////////////////////
                        Sleep(1000);
                        copyServiceSocket = Connector(COPY_SERVER_IP_ADDRESS, 7000);
                        if (copyServiceSocket == INVALID_SOCKET) {
                            printf("\n\nFAILED TO RECONNECT TO COPY SERVICE!");
                            stopFlag = true;
                            
                        }
                    }

                  
                
            }
           
        }
        if (copyServiceSocket != INVALID_SOCKET) {
            closesocket(copyServiceSocket);
            printf("\n\t\t\tService socket closed");
        }
        if (socket != INVALID_SOCKET) {
            closesocket(socket);
            printf("\n\t\t\tProcess socket closed");
        }
    }

}
/////////////

DWORD WINAPI handleBackupToClient(LPVOID lpParam) {
    Measurement m;
    SOCKET socket;
    
    while (true) {
        std::unique_lock<std::mutex> lock(backupCMutex);
        backupConditionClient.wait(lock, [] { return !is_req_queue_empty(&backupConnections) || stopFlag == true; });

        printf("\n\t\t\tBackup thread started, searching backup...");
            
        if (stopFlag == false) {
            socket = dequeue_request(&backupConnections);
                if (socket == INVALID_SOCKET)continue;
            }
            else {
                printf("\n\t\t\t\tThread finished.");
                return 1;
            }

        insert_mtx_to_map(&queueMtxsBackup, currentBackupChoice);
        std::mutex* mtxForBackup = get_mtx_for_key(&queueMtxsBackup, currentBackupChoice);

        if (mtxForBackup == NULL) {
            std::cerr << "Mutex for key " << currentBackupChoice << " not found!\n";
            return -1;
        }

            {
                std::unique_lock<std::mutex> lock(*mtxForBackup);
                if (contains_key(&messageQueuesBackup, currentBackupChoice)) {
                    queue* currBackupQ = NULL;
                    currBackupQ = get_queue_for_key(&messageQueuesBackup, currentBackupChoice);

                    while (!is_queue_empty(currBackupQ)) {
                        dequeue(currBackupQ, &m);
                        
                            if (totalBackup % 1000 == 0)
                                printf("\n\t\t\t\tSending backup to process: %d", totalBackup.load());
                        
                            totalBackup--;
                        if (!SendMessageTo(socket, m)) {

                            enqueue(currBackupQ, m);
                            totalBackup++;
                        }

                    }
                    printf("\n\t\t\tBackup sent.");
                    free_queue(currBackupQ);
                }
                else {
                    printf("\n\t\t\tQueue is empty");
                    
                }
            }
            printf("\n\t\t\tBackup thread finished.");
            closesocket(socket);
    }
}


///
DWORD WINAPI getMessagesThread(LPVOID lpParam) {
    SOCKET backupRequest = INVALID_SOCKET;
    int iResult;
    int numOfProcesses=0;
    Measurement m;
    int counter = 0;
    int localCounter = 0;
    bool localStopFlag = false;

    while (!stopFlag) {
        
        {
            std::unique_lock<std::mutex> lock(backupMutex);
          //  printfP("\nWaiting for backup condition.. [\tPress enter for menu!!!!\t]", nullptr, totalMessages.load(), totalBackup.load());
            backupCondition.wait(lock, [] {return backup == true || stopFlag; });
        }
        if (stopFlag)break;

            backupRequest = Connector(COPY_SERVER_IP_ADDRESS, 9000);
               if (backupRequest == INVALID_SOCKET) {
                   printf("\nCant connect to copy service...");
                   closesocket(backupRequest); continue;
                }

            m.deviceId = currentBackupChoice;
            m.purpose = 0;
        
            if (stopFlag) {
                printf("\nServer shutting down, backup thread exiting...\n");
                break;
            }
            if (SendMessageTo(backupRequest, m)) {
                printf("\nRequest sent...\n");
            }
            else {
                printf("\nFailed to send.");
                continue;
            }
           
        
        printf("\n\t\t\tReceiving and printing backup for ID %d...",m.deviceId);
        makeSocketNonBlocking(backupRequest);
        bool stop = false;
    
        //std::atomic<bool> localStopFlag;
        while (!stop) {
            
            insert_mtx_to_map(&queueMtxsBackup, currentBackupChoice);
            std::mutex* mtxForBackup = get_mtx_for_key(&queueMtxsBackup, currentBackupChoice);

            if (mtxForBackup == NULL) {
                std::cerr << "Mutex for key " << currentBackupChoice << " not found!\n";
                return -1;
            }

            ReceiveAndFilterMessages(stopFlag, m, backupRequest, totalBackup, stop, localCounter, messageQueuesBackup, *mtxForBackup);
            }

        if (clientInitiated) {
            backupConditionClient.notify_one();
            clientInitiated = false;
        }
        printf("\n\t\t\tTotal messages from backup: %d \n", totalBackup.load());
        backup = false;
        closesocket(backupRequest);
     
    }
    printf("\n\t\t\tBackup thread stopped");
    
    if(backupRequest!=INVALID_SOCKET)
     closesocket(backupRequest);
    
    return 1;

}







void InputCommands() {
    char ch;
    while (true) {

        ch = std::getchar();
        const char* commandMenu = "\n\t\t\t\t(1) <-- Backup\n\t\t\t\t(2) <-- Print more info\n\t\t\t\t(3) <-- Print queue [Temporary/Backup]\n\t\t\t\t(4) <-- Exit\n";
        printfP(commandMenu, nullptr, totalMessages.load(), totalBackup.load());
        // std::cin >> ch;

        if (ch == '4')
        {
            printf("\n\n\t\t\t\tExiting\n");
            break;
        }
        else if (ch == '1') {

            const char* cop = "\n\t\t\t\t(1) <-- Backup for process 1\n\t\t\t\t(2) <-- Backup for process 2\n\t\t\t\t(3) <-- Backup for process 3\n";
            printfP(cop, nullptr, totalMessages.load(), totalBackup.load());
            std::cin >> currentBackupChoice;
            backup = true;
            backupCondition.notify_one();

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
        else if(ch=='3'){
           // printer(messageQueues, queueMtxs);
              //printQueueSelection(messageQueues, queueMtxs, messageQueuesBackup, queueMtxsBackup, ch);
          
        }
        else {

            continue;
        }
    
    }

}






DWORD WINAPI acceptorThread(LPVOID lpParam) {
    Params pars = *(Params*)lpParam;
   // SOCKET serverSocket = *(SOCKET*)lpParam;
    makeSocketNonBlocking(pars.serverSocket);
    
    while (!stopFlag) {

        SOCKET clientSocket = InitializeAcceptor(stopFlag, pars.serverSocket);
  
        if (clientSocket != INVALID_SOCKET)
        {
            if (pars.id == 1) {
                std::cout << "\nClient connected!" << std::endl;
                //printSockaddr(clientAddr);
                {
                    std::unique_lock<std::mutex> lock(socketMutex);
                    enqueue_request(&socketQueue, clientSocket);
                    dataCondition.notify_one();
                }
            }
            else if (pars.id == 2) {
                {
                    std::cout << "\nProcess for backup connected" << std::endl;
                    std::unique_lock<std::mutex> lock(backupCMutex);
                    enqueue_request(&backupConnections, clientSocket);
                   // backupConditionClient.notify_one();
                }
            }

        }
        else {
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


        }
    }
   
    if(pars.serverSocket!=INVALID_SOCKET)
       closesocket(pars.serverSocket);
   
    printf("\nAcceptor thread exiting...");
    return 0;
}



int starting() {
    init_map(&messageQueues);
    init_map(&messageQueuesBackup);

    init_mtx_map(&queueMtxs);
    init_mtx_map(&queueMtxsBackup);
    
    init_req_queue(&socketQueue, 16);
    init_req_queue(&backupConnections, 16);
    SOCKET serverSocket = INVALID_SOCKET;
    SOCKET backupListener = INVALID_SOCKET;
    InitializeWindowsSockets();
    serverSocket = initializeListener(SERVER_IP_ADDRESS, "8000");
    backupListener = initializeListener(SERVER_IP_ADDRESS, "8500");
    if (serverSocket == INVALID_SOCKET || backupListener == INVALID_SOCKET) { printf("\n\n\t\t\tCan't initialize server, shutting down..."); return 1; }
    printf("\nServer initialized, waiting for clients.\n");
    Params p2;
    Params p1;
    p1.serverSocket = serverSocket;
    p1.id = 1;
    p2.serverSocket = backupListener;
    p2.id = 2;
    printf("\n\n\t\t\t\tPress Enter for Menu...\n");
    // Create thread pool
    const int THREAD_POOL_SIZE = 10;
    HANDLE thread_pool[THREAD_POOL_SIZE];
    HANDLE acceptor = CreateThread(nullptr, 0, acceptorThread,&p1, 0, nullptr);
    HANDLE acceptorBackup = CreateThread(nullptr, 0, acceptorThread,&p2, 0, nullptr);
    HANDLE backupHandler = CreateThread(nullptr, 0, handleBackupToClient, nullptr, 0, nullptr);
    HANDLE createThread = CreateThread(nullptr, 0, getMessagesThread, nullptr, 0, nullptr);

    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        thread_pool[i] = CreateThread(nullptr, 0, handleProcesses, &messageQueues, 0, nullptr);
    }
    // Start acceptor thread
    InputCommands();
    // Wait for user input to shut down the server
    std::cout << "\nStopping the server..." << std::endl;

    {
        std::lock_guard<std::mutex> lock(socketMutex);
        stopFlag = true;
    }
    dataCondition.notify_all();
    backupCondition.notify_all();

    // Wait for threads to exit
    WaitForSingleObject(createThread, INFINITE);
    WaitForMultipleObjects(THREAD_POOL_SIZE, thread_pool, TRUE, INFINITE);
    WaitForSingleObject(acceptor, INFINITE);
    printf("\n\t\t\t\tClean up.............\n");     // Cleanup
    Sleep(200);
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        CloseHandle(thread_pool[i]);
    }
    
    //
    free_map(&messageQueues);
    /*
    for (auto i :messageQueues) {
       // print_queue(&(i.second));
        printf("\n\tFreeing what is left in temporary queue %d\n",i.first);
        {
            std::unique_lock<std::mutex> lock(queueMtxs[i.first]);
            if (!is_queue_empty(&(i.second))) {
                    free_queue(&(i.second));
            }
        }
    }
    */
    free_map(&messageQueuesBackup);
    /*
    for (auto i : messageQueuesBackup) {
        // print_queue(&(i.second));
        printf("\n\tFreeing what is left in backup queue %d\n", i.first);
        {
            std::unique_lock<std::mutex> lock(queueMtxsBackup[i.first]);
            if (!is_queue_empty(&(i.second))) {
                free_queue(&(i.second));
            }
        }
    }
    */
    free_mtx_map(&queueMtxs);
    free_mtx_map(&queueMtxsBackup);
    
    

    while (!is_req_queue_empty(&socketQueue)) {
        if (dequeue_request(&socketQueue) != INVALID_SOCKET) {
            closesocket(dequeue_request(&socketQueue));
        }
    }

    free_req_queue(&socketQueue);
    free_req_queue(&backupConnections);

   if(acceptor!=INVALID_HANDLE_VALUE)
    CloseHandle(acceptor);
   if(createThread!=INVALID_HANDLE_VALUE)
    CloseHandle(createThread);
   if (serverSocket != INVALID_SOCKET)
    closesocket(serverSocket);

    WSACleanup();
    std::cout << "\n\n\n\t\t\t\t\tDone. Server shut down." << std::endl;
    Sleep(1000);

    return 0;


}


int main() {

starting();
return 1;
}

