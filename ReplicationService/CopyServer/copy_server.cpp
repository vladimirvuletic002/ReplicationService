#include "../Server/common.h"

bool InitializeWindowsSockets();

DWORD WINAPI replicationHandler(LPVOID param) {
    MessageQueue* queue = (MessageQueue*)param;
    SOCKET copySocket;
    struct sockaddr_in copyAddr;
    char message[MAX_MESSAGE_LENGTH];

    copySocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    copyAddr.sin_family = AF_INET;
    copyAddr.sin_port = htons(8081);
    copyAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    connect(copySocket, (struct sockaddr*)&copyAddr, sizeof(copyAddr));

    while (1) {
        if (dequeue(queue, message)) {
            send(copySocket, message, strlen(message), 0);
            printf("Prosledjeno kopiji servera: %s\n", message);
        }
    }

    closesocket(copySocket);
    return 0;
}

int startCopyServer() {

    SOCKET listenSocket = INVALID_SOCKET;
    SOCKET clientSocket = INVALID_SOCKET;

    if (InitializeWindowsSockets() == false)
    {
        // Logovanje je odradjeno u InitializeWindowsSockets() f-ji
        return 1;
    }

    struct addrinfo* result = NULL, hints;


    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(NULL, "8081", &hints, &result);
    listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    bind(listenSocket, result->ai_addr, (int)result->ai_addrlen);
    listen(listenSocket, SOMAXCONN);
    freeaddrinfo(result);

    printf("Kopija servera slusa na portu 8081\n");
    while (1) {
        clientSocket = accept(listenSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) continue;

        Measurement data;
        int bytesReceived = recv(clientSocket, (char*)&data, sizeof(Measurement), 0);
        if (bytesReceived > 0) {
            printf("Sacuvani podaci uredjaja: ID=%d, Jacina struje=%.2f Napon=%.2f Snaga=%.2f\n",
                data.deviceId, data.current, data.voltage, data.power);
        }
        closesocket(clientSocket);
    }

    closesocket(listenSocket);
    WSACleanup();
    return 0;
}

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