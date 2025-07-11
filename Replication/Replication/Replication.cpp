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




void InitializeQu(Measurement data,int &currentId) {
    
    if (data.deviceId > 0 && data.deviceId <= NUMBER_OF_POSSIBLE_IDS) {

        if (!contains_key(&messageQueues, data.deviceId)) {
            queue q;
            init_queue(&q);
            insert_queue_to_map(&messageQueues, data.deviceId, &q);
            insert_mtx_to_map(&queueMtxs, data.deviceId);
        }
        currentId = data.deviceId;
    }
}



/////////////
DWORD WINAPI handleProcesses(LPVOID lparm) {
    SOCKET socket;
    SOCKET copyServiceSocket = INVALID_SOCKET;
    int currentId = -1;
    Measurement data;
    queue* messageQueue = (queue*)lparm;

    while (true) {
        {
            currentId = -1;
            std::unique_lock<std::mutex> lock(socketMutex);
            dataCondition.wait(lock, [] { return !is_req_queue_empty(&socketQueue) || stopFlag; });

            backup = false;

            if (!stopFlag) {
                socket = dequeue_request(&socketQueue);
                if (socket == INVALID_SOCKET)
                    continue;
            }
            else {
                printf("\n\t\t\t\tThread finished.");
                if (copyServiceSocket != INVALID_SOCKET)
                    closesocket(copyServiceSocket);
                return 1;
            }
        }
        makeSocketNonBlocking(socket);

        while (true) {
            int bytesReceived = recv(socket, (char*)&data, sizeof(Measurement), 0);

            if (bytesReceived <= 0 || stopFlag) {
                if (stopFlag) {
                    if (copyServiceSocket != INVALID_SOCKET)
                        closesocket(copyServiceSocket);
                    if (socket != INVALID_SOCKET)
                        closesocket(socket);
                    printf("\n\n\t\t\t\tForce server shutdown! ID ---> %d\n\b", currentId);
                    //break;
                    return 1;
                }
                int error = WSAGetLastError();
                if (error == WSAEWOULDBLOCK) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(150));
                    continue;
                }
                else {
                    if (copyServiceSocket != INVALID_SOCKET)
                        closesocket(copyServiceSocket);
                    if (socket != INVALID_SOCKET)
                        closesocket(socket);
                    std::cerr << "\n\t\t\tConnection with process closed : Error: " << error << '\n';
                    break;
                }
            }

            if (currentId == -1) {
                InitializeQu(data, currentId);
            }

            if (currentId == data.deviceId) {
                if (data.purpose == 99) { // Client initiated backup
                    printf("\nCLIENT INITIATED BACKUP");
                    currentBackupChoice = data.deviceId;
                    backup = true;
                    clientInitiated = true;
                    backupCondition.notify_one();
                    continue;
                }

                insert_mtx_to_map(&queueMtxs, data.deviceId);
                std::mutex* mtxForDevice = get_mtx_for_key(&queueMtxs, data.deviceId);
                queue* qForDevice = get_queue_for_key(&messageQueues, data.deviceId);

                if (mtxForDevice == NULL || qForDevice == NULL) {
                    std::cerr << "Mutex or queue for key " << data.deviceId << " not found!\n";
                    continue;
                }

                {
                    std::unique_lock<std::mutex> lock(*mtxForDevice);
                    enqueue(qForDevice, data); // UVEK enqueue!
                }
                
            }
        }
        /*if (copyServiceSocket != INVALID_SOCKET) {
            closesocket(copyServiceSocket);
            printf("\n\t\t\tService socket closed");
        }
        if (socket != INVALID_SOCKET) {
            closesocket(socket);
            printf("\n\t\t\tProcess socket closed");
        }*/
        //closesocket(socket);
        //printf("\n\t\t\tProcess socket closed");
    }
    return 1;
}


DWORD WINAPI QueueDeliveryThread(LPVOID lpParam) {
    // Za svaki device, drži poseban socket
    SOCKET deviceSockets[NUM_DEVICES + 1] = { INVALID_SOCKET }; // deviceId ide od 1!
    while (!stopFlag) {
        for (int deviceId = 1; deviceId <= NUM_DEVICES; ++deviceId) {
            std::mutex* mtx = get_mtx_for_key(&queueMtxs, deviceId);
            queue* q = get_queue_for_key(&messageQueues, deviceId);
            if (!mtx || !q) continue;

            while (true) {
                Measurement m;
                {
                    std::unique_lock<std::mutex> lock(*mtx);
                    if (is_queue_empty(q)) break;
                    m = q->head->data;
                }

                // PERSISTENT CONNECTION: koristi isti socket dok ne pukne!
                if (deviceSockets[deviceId] == INVALID_SOCKET) {
                    deviceSockets[deviceId] = Connector(COPY_SERVER_IP_ADDRESS, 7000);
                    if (deviceSockets[deviceId] == INVALID_SOCKET) {
                        printf("\n[QUEUE->COPY] CopyService unavailable (dev=%d), sleeping 1s...\n", deviceId);
                        std::this_thread::sleep_for(std::chrono::seconds(10));
                        break;
                    }
                    printf("\n[QUEUE->COPY] Opened persistent connection for device %d\n", deviceId);
                }
                SOCKET s = deviceSockets[deviceId];

                if (SendMessageTo(s, m)) {
                    // ACK stigao, sad izbrisi poruku iz reda
                    {
                        std::unique_lock<std::mutex> lock(*mtx);
                        Measurement tmp; dequeue(q, &tmp);
                    }
                    if (m.purpose != 8) {
                        printfP("New manually sent data", &m, totalMessages.load(), totalBackup.load());
                    }
                    {
                        std::unique_lock<std::mutex> lock(messageCounter);
                        totalMessages++;
                        if (totalMessages % 1000 == 0)
                            printf("\n\t\t\t\tTotal messages received = %d\n\t\t\t\tSuccessfully forwarded to copy server.\n", totalMessages.load());
                    }
                }
                else {
                    // greska -- ZATVORI socket i postavi na INVALID_SOCKET, pa sledeci put radi reconnect!
                    closesocket(s);
                    deviceSockets[deviceId] = INVALID_SOCKET;
                    printf("\n[QUEUE->COPY] Failed to send, will retry after reconnect (dev=%d)\n", deviceId);
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    break;
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    for (int i = 1; i <= NUM_DEVICES; ++i) {
        if (deviceSockets[i] != INVALID_SOCKET)
            closesocket(deviceSockets[i]);
    }
    return 0;
}



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
                        
                            
                            //if (!SendMessageTo(socket, m)) {
                            if(send(socket, (const char*)&m, sizeof(Measurement), 0) <=0){
                            enqueue(currBackupQ, m);
                            }
                            else {
                                totalBackup--;
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
    std::string input;
    while (true) {
        const char* commandMenu = "\n\t\t\t\t(1) <-- Backup\n\t\t\t\t(2) <-- Print more info\n\t\t\t\t(3) <-- Print queue [Temporary/Backup]\n\t\t\t\t(4) <-- Exit\n";
        printfP(commandMenu, nullptr, totalMessages.load(), totalBackup.load());

        std::cout << "\nChoose: ";

        std::getline(std::cin, input);

        if (input == "4") {
            printf("\n\n\t\t\t\tExiting\n");
            break;
        }
        else if (input == "1") {
            const char* cop = "\n\t\t\t\t(1) <-- Backup for process 1\n\t\t\t\t(2) <-- Backup for process 2\n\t\t\t\t(3) <-- Backup for process 3\n";
            printfP(cop, nullptr, totalMessages.load(), totalBackup.load());
            
            std::getline(std::cin, input);

            if (input == "1") currentBackupChoice = 1;
            else if (input == "2") currentBackupChoice = 2;
            else if (input == "3") currentBackupChoice = 3;
            else {
                printf("\n\t\t\tInvalid backup choice!\n");
                continue;
            }

            backup = true;
            backupCondition.notify_one();
        }
        else if (input == "2") {
            info = !info;
            if (info) {
                printf("\n\t\t\t\t--------------------------\n\t\t\t\tMore feedback turned ON\n\t\t\t\t--------------------------\n");
            }
            else {
                printf("\n\t\t\t\t--------------------------\n\t\t\t\tMore feedback turned OFF\n\t\t\t\t--------------------------\n");
            }
        }
        else if (input == "3") {
            printQueueSelection(messageQueues, queueMtxs, messageQueuesBackup, queueMtxsBackup, '\0');
        }
        else {
            printf("\n\t\t\tInvalid choice!\n");
            continue;
        }
    }
}






DWORD WINAPI acceptorThread(LPVOID lpParam) {
    Params pars = *(Params*)lpParam;
    makeSocketNonBlocking(pars.serverSocket);
    
    while (!stopFlag) {

        SOCKET clientSocket = InitializeAcceptor(stopFlag, pars.serverSocket);
  
        if (clientSocket != INVALID_SOCKET)
        {
            if (pars.id == 1) {
                std::cout << "\nClient connected!" << std::endl;
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
                    //closesocket(clientSocket);
                    //printf("\n\nError: %d. Shutting down...\n", errCode);
                    //break;
                    //continue;
                    if (clientSocket != INVALID_SOCKET)
                        closesocket(clientSocket);
                    printf("\n\n[acceptor] Error on accept(): %d\n", errCode);
                    // NE gasi serverSocket, NE izlazi iz thread-a, SAMO nastavi dalje!
                    continue;
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
    HANDLE queueDeliveryThread = CreateThread(nullptr, 0, QueueDeliveryThread, nullptr, 0, nullptr);

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
    backupConditionClient.notify_all();

    // Wait for threads to exit
    WaitForSingleObject(createThread, INFINITE);
    WaitForMultipleObjects(THREAD_POOL_SIZE, thread_pool, TRUE, INFINITE);
    WaitForSingleObject(acceptor, INFINITE);
    WaitForSingleObject(acceptorBackup, INFINITE);
    WaitForSingleObject(backupHandler, INFINITE);
    WaitForSingleObject(queueDeliveryThread, INFINITE);
    

    CloseHandle(acceptor);
    CloseHandle(createThread);
    CloseHandle(queueDeliveryThread);
    CloseHandle(acceptorBackup);
    CloseHandle(backupHandler);

    printf("\n\t\t\t\tClean up.............\n");     // Cleanup
    Sleep(200);
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        CloseHandle(thread_pool[i]);
    }
    
    
    free_map(&messageQueuesBackup);
    free_map(&messageQueues);
    free_mtx_map(&queueMtxs);
    free_mtx_map(&queueMtxsBackup);
    
    

    while (!is_req_queue_empty(&socketQueue)) {
        SOCKET sock = dequeue_request(&socketQueue);
        if (sock != INVALID_SOCKET) {
            closesocket(sock);
        }
    }

    while (!is_req_queue_empty(&backupConnections)) {
        SOCKET sock = dequeue_request(&backupConnections);
        if (sock != INVALID_SOCKET) {
            closesocket(sock);
        }
    }

    free_req_queue(&socketQueue);
    free_req_queue(&backupConnections);
       
   
   if (serverSocket != INVALID_SOCKET)
    closesocket(serverSocket);

   if (backupListener != INVALID_SOCKET)
       closesocket(backupListener);

    WSACleanup();
    std::cout << "\n\n\n\t\t\t\t\tDone. Server shut down." << std::endl;
    Sleep(1000);

    return 0;


}


int main() {

starting();
return 1;
}

