﻿#include "Common.h"
#include <unordered_map>
#include <iostream>
#include <mutex>


#define DEFAULT_BUFFLEN 512
#define RECEIVE_REFRESH 100
std::mutex mtx;

bool InitializeWindowsSockets()
{
    WSADATA wsaData;
    // Initialize windows sockets library for this process
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        printf("WSAStartup failed with error: %d\n", WSAGetLastError());
        return false;
    }
    return true;
}
SOCKET initializeListener(const char* ip_address, const char* port) {

    // Socket used for listening for new clients
    SOCKET serverSocket = INVALID_SOCKET;
    // Socket used for communication with client
    SOCKET acceptedSocket = INVALID_SOCKET;
    // variable used to store function return value
    int iResult;
    // Buffer used for storing incoming data
    char recvbuf[DEFAULT_BUFFLEN];

    // Prepare address information structures
    addrinfo* resultingAddress = NULL;
    addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       // IPv4 address
    hints.ai_socktype = SOCK_STREAM; // Provide reliable data streaming
    hints.ai_protocol = IPPROTO_TCP; // Use TCP protocol
    hints.ai_flags = AI_PASSIVE;     // 
    //  const char* port = "7000";
     // const char* ip_address = "127.0.0.3";
      // process(port);


    
    // std::cin.get();

     // Resolve the server address and port
    iResult = getaddrinfo(NULL, port, &hints, &resultingAddress);
    if (iResult != 0)
    {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return INVALID_SOCKET;
    }





    // Set the IP address manually using inet_pton
    iResult = inet_pton(AF_INET, ip_address, &hints.ai_addr);
    if (iResult <= 0) {
        printf("inet_pton failed with error: %d\n", WSAGetLastError());
        WSACleanup();
        return INVALID_SOCKET;
    }


    // Create a SOCKET for connecting to server
    serverSocket = socket(AF_INET,      // IPv4 address famly
        SOCK_STREAM,  // stream socket
        IPPROTO_TCP); // TCP

    if (serverSocket == INVALID_SOCKET)
    {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(resultingAddress);
        WSACleanup();
        return INVALID_SOCKET;
    }

    // Setup the TCP listening socket - bind port number and local address 
    // to socket
    iResult = bind(serverSocket, resultingAddress->ai_addr, (int)resultingAddress->ai_addrlen);
    if (iResult == SOCKET_ERROR)
    {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(resultingAddress);
        closesocket(serverSocket);
        WSACleanup();
        return INVALID_SOCKET;
    }

    // Since we don't need resultingAddress any more, free it
    freeaddrinfo(resultingAddress);

    // Set listenSocket in listening mode
    iResult = listen(serverSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR)
    {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(serverSocket);
        WSACleanup();
        return INVALID_SOCKET;
    }

    printf("\nServer initialized, waiting for clients. [");
    std::cout << "    "<<ip_address;
    std::cout << "  " << port<<" ]";
    return serverSocket;

}



SOCKET Connector(const char* SERVICE_IP_ADDRESS, u_short port) {

    SOCKET connectSocket = INVALID_SOCKET;
    Measurement measurement;
    int iResult;
    int choice;

    connectSocket = socket(AF_INET,
        SOCK_STREAM,
        IPPROTO_TCP);

    if (connectSocket == INVALID_SOCKET)
    {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return INVALID_SOCKET;
    }

   
    // create and initialize address structure

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(SERVICE_IP_ADDRESS);
    serverAddress.sin_port = htons(port);

    // connect to server specified in serverAddress and socket connectSocket
    if (connect(connectSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
    {
        printf("Unable to connect to server.\n");
        closesocket(connectSocket);
        WSACleanup();
        return INVALID_SOCKET;
    }

    return connectSocket;
}

SOCKET InitializeAcceptor(std::atomic<bool> &stopFlag,SOCKET serverSocket) {

    while (!stopFlag) {

        sockaddr_in clientAddr;
        int clientAddrLen = sizeof(clientAddr);

        SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrLen);
        if (clientSocket == INVALID_SOCKET) {

            if (stopFlag) {
                closesocket(serverSocket);
                closesocket(clientSocket);
                break;
            }
            if (WSAGetLastError() == WSAEWOULDBLOCK) {
                // No connection attempt; continue the loop
                // Avoid busy-waiting
                Sleep(250);
                continue;
            }
            else {
                std::cerr << "accept failed shutting down: " << WSAGetLastError() << '\n';
               Sleep(10000);
               stopFlag = true;
               break;
            }
        }
        std::cout << "Client connected!" << std::endl;
        u_long mode = 0; // 0 for blocking mode
        ioctlsocket(clientSocket, FIONBIO, &mode);
        // Add the client socket to the queue
        // }
        return clientSocket;
    }
}






bool SendMessageTo(SOCKET socket, Measurement data) {

    int iResult = send(socket, (const char*)&data, sizeof(Measurement), 0);

    if (iResult > 0) {
        return true;
    }
    else {
        return true;
    }
}

bool isQueueInitialized(const queue& q) {
    // A queue is considered initialized if head, tail are null and size is zero.
    return (q.head == nullptr && q.tail == nullptr && q.size == 0);
}

bool SendMessageQ(SOCKET copyServiceSocket, Measurement data, queue *messageQueue, std::mutex& queueMtx) {
    {
        std::unique_lock<std::mutex> lock(queueMtx);
        if (enqueue(messageQueue, data)) {// Dodaj poruku u red za prosleđivanje kopiji
          
          //  printf("\nTemporary saving message for device ID: %d", data.deviceId);
        }
        else {
            printf("\nFAILED TO ENQUEUE");
        }
        if (SendMessageTo(copyServiceSocket, data)) {
            //printf("\nSuccess sending mess\n");
                // Needs to dequeue from tail???
                return true;
            }
       
        

    }
    return false;
}

bool EnqueueToMap(std::mutex &queueMtx, unordered_map &messageQueues, Measurement data) {

    {
        std::unique_lock<std::mutex> lock(queueMtx);
        if(contains_key(&messageQueues, data.deviceId)){
            queue* found = NULL;
            found = get_queue_for_key(&messageQueues, data.deviceId);

            if (enqueue(found, data)) {// Dodaj poruku u red za prosleđivanje kopiji
                

                // printf("\nMessage enqueued, notifying...");
                return true;
                //dataCondition[data.deviceId].notify_all();
            }
        }
        else {
            queue *newQ;
            init_queue(newQ);
            insert_queue_to_map(&messageQueues, data.deviceId, newQ);
            if (enqueue(newQ, data)) {
             //printf("\nMessage enqueued, notifying id %d...", data.deviceId);
                return true;
               // dataCondition[data.deviceId].notify_all();
            }
        }
        return false;
    }



}


int ReceiveWithNonBlockingSocket(std::atomic<bool>& stopFlag, SOCKET& socket, Measurement& m) {

    int bytesReceived = recv(socket, (char*)&m, sizeof(Measurement), 0);

    if (stopFlag) {
        return END;
    }
    
    if (bytesReceived <= 0) {

        int error = WSAGetLastError();
        if (error == WSAEWOULDBLOCK) {
            // No data available yet; wait and retry
            std::this_thread::sleep_for(std::chrono::milliseconds(RECEIVE_REFRESH));
            return SKIP;
        }
        else {
            std::cerr << "\n\t\t\t\t\tConnection closed!"<< '\n';
            return END;
        }
    }
    else {
        return PROCEED;
    }

}

 int nonBlockingReceive(std::atomic<bool>& stopFlag, SOCKET& socket, Measurement& data, std::atomic<bool>& stop,bool print){

    switch (ReceiveWithNonBlockingSocket(stopFlag, socket, data)) {

    case END:
        stop = true;
        return END;

    case SKIP:
        return SKIP;

    case PROCEED:
      
        if (print && data.purpose!=-1) {
            printf("\n\n------- New Data received ---------\n");
            printMeasurement(&data);
         
        }
     
    }
    return 0;

 }
 
 void printQueue(queue &q,std::mutex& mtx) {
     if (!is_queue_empty(&q)) {
         
         if (q.size > 1000) {
             printf("\nNOTICE: Queue size has more than 1000 messages, are you sure you want to print it? Y/N\nQueue size: %d\n",q.size);
             char c;
             std::cin >> c;
             if (c == 'Y' || c=='y') {
                 printf("\n\t\t\t\tThis may take a long time...");
                 Sleep(2000);
             }
             else {
                 printf("\n\t\t\t\tCanceled printing\n");
                 return;

             }
            
         }
         {
             std::unique_lock<std::mutex> lock(mtx);
             print_queue(&q);
         }
     }
     else {
         printf("\n\t\t\t\tQueue is empty!\n");
     }
}

 void printer(std::unordered_map<int, queue>& messageQueues, std::unordered_map<int, std::mutex>& queueMtxs) {
     printf("\n\t\t\t\t(1) Queue ID1");
     printf("\n\t\t\t\t(2) Queue ID2");
     printf("\n\t\t\t\t(3) Queue ID3\n");
     char c;
     std::cin >> c;
     if (c == '1') {
         printQueue(messageQueues[1], queueMtxs[1]);
     }
     else if (c == '2') {
         printQueue(messageQueues[2], queueMtxs[2]);

     }
     else if (c == '3') {
         printQueue(messageQueues[3], queueMtxs[3]);

     }
     else {
         printf("\n\t\t\tInvalid choice!");
     }
 }


 void printQueueSelection(unordered_map &messageQueues,std::unordered_map<int,std::mutex>& queueMtxs, unordered_map &messageQueuesBackup, std::unordered_map<int, std::mutex>& queueMtxsBackup, char c) {
   
     printf("\n\n\t\t\t\t(1) <-- Temporary Queues");
     printf("\n\t\t\t\t(2) <-- Backup Queues\n");
     std::cin >> c;

     if(c=='1'){
         
        printer(messageQueues,queueMtxs);
     }
    
    if (c == '2') {
        printer(messageQueuesBackup, queueMtxsBackup);
    }
    else {
        printf("\nInvalid choice!");
    }
 }

bool SafeDequeue(queue *queue, std::mutex& qMtx, Measurement* m) {

    {
        std::unique_lock<std::mutex> lock(qMtx);
        if (dequeue(queue, m)) {
            printf("\nMessage enqueued.");
            return true;
        }
        return false;
    }


}

bool SafeEnqueue(queue *queue, std::mutex& qMtx, Measurement m) {

    {
        std::unique_lock<std::mutex> lock(qMtx);
        if (enqueue(queue, m)) {
            printf("\nMessage enqueued.");
            return true;
        }
        return false;
    }


}

void printMeasurement(Measurement* m) {
    char time_string[50];
    strftime(time_string, sizeof(time_string), "%Y-%m-%d %H:%M:%S", &m->timeStamp);
    printf("ID uredjaja: %d\n", m->deviceId);
    if (m->type == CURRENT) {
        printf("Jacina struje: %.2f [A]\n", m->value);
    }

    else if (m->type == VOLTAGE) {
        printf("Napon: %.2f [V]\n", m->value);
    }

    else {
        printf("Snaga: %.2f [kW]\n", m->value);
    }

    printf("Vreme: %s\n", time_string);
}



