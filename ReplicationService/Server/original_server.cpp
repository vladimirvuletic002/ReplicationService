#include "common.h"
#include "queue.h"

bool InitializeWindowsSockets();
DWORD WINAPI clientHandler(LPVOID param);
int startOriginalServer(queue* queue);

int main() {
    // Inicijalizacija reda
    queue messQueue;
    init_queue(&messQueue);

    printf("Pokretanje original servera na portu %s...\n", DEFAULT_PORT);

    // Pokretanje servera
    int result = startOriginalServer(&messQueue);
    if (result != 0) {
        printf("Greska pri pokretanju servera!\n");
        return EXIT_FAILURE;
    }

    // Ovde možeš dodati logiku za dodatnu obradu podataka u redu,
    // ako je potrebno (npr. ispisivanje redova, simulacija replikacije itd.)

    // Kada završiš sa korišćenjem reda, oslobodi memoriju
    free_queue(&messQueue);

    return 0;
}


DWORD WINAPI clientHandler(LPVOID param) {
    SOCKET clientSocket = *(SOCKET*)param;
    queue* messQueue = (queue*)((SOCKET*)param + 1);
    Measurement data;
    init_queue(messQueue);

    while (1) {
        int bytesReceived = recv(clientSocket, (char*)&data, sizeof(Measurement), 0);
        if (bytesReceived <= 0) break;

        printf("\n\n---------Podaci primljeni od uredjaja:---------\n ID: %d\n Jacina struje=%.2f [A]\n Napon=%.2f [V]\n Snaga=%.2f [kW]\n-----------------------------------------------\n",
            data.deviceId, data.current, data.voltage, data.power);

        char message[MAX_MESSAGE_LENGTH];
        snprintf(message, MAX_MESSAGE_LENGTH, "Uredjaj ID: %d, Jacina struje=%.2f, Napon=%.2f, Snaga=%.2f",
            data.deviceId, data.current, data.voltage, data.power);

        enqueue(messQueue, data); // Dodaj poruku u red za prosleđivanje kopiji
    }

    closesocket(clientSocket);
    return 0;
}

int startOriginalServer(queue* queue) {

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

    getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    bind(listenSocket, result->ai_addr, (int)result->ai_addrlen);
    listen(listenSocket, SOMAXCONN);
    freeaddrinfo(result);

    printf("Original server listening on port %s\n", DEFAULT_PORT);
    while (1) {
        clientSocket = accept(listenSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) continue;

        HANDLE thread = CreateThread(NULL, 0, clientHandler, &clientSocket, 0, NULL);
        CloseHandle(thread);
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