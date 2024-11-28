#include "../Server/common.h"
#include "../Server/queue.h"

bool InitializeWindowsSockets();
DWORD WINAPI clientHandler(LPVOID param);
int startProcess(int port, queue* queue);

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    queue messQueue;
    init_queue(&messQueue);

    // Pokretanje servera
    int result = startProcess(port, &messQueue);
    if (result != 0) {
        printf("Greska pri pokretanju servera!\n");
        return EXIT_FAILURE;
    }

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

        printf("\n\n---------Podaci primljeni od uredjaja:---------\n");
        printMeasurement(&data);


        /*
        * char message[MAX_MESSAGE_LENGTH];
        snprintf(message, MAX_MESSAGE_LENGTH, "Uredjaj ID: %d, Jacina struje=%.2f, Napon=%.2f, Snaga=%.2f",
            data.deviceId, data.current, data.voltage, data.power);
        */

        enqueue(messQueue, data); // Dodaj poruku u red za prosleđivanje kopiji
        //print_queue(messQueue);
    }

    closesocket(clientSocket);
    return 0;
}

int startProcess(int port, queue* messQueue) {
    SOCKET listenSocket = INVALID_SOCKET;
    SOCKET clientSocket = INVALID_SOCKET;

    if (InitializeWindowsSockets() == false) {
        return 1;
    }

    struct addrinfo* result = NULL, hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Poveži na odgovarajući port
    char portStr[6];
    snprintf(portStr, sizeof(portStr), "%d", port);
    getaddrinfo(NULL, portStr, &hints, &result);

    listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    bind(listenSocket, result->ai_addr, (int)result->ai_addrlen);
    listen(listenSocket, SOMAXCONN);
    freeaddrinfo(result);

    printf("Server pokrenut na portu %d. Cekanje na klijente...\n", port);

    while (1) {
        clientSocket = accept(listenSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) continue;

        // Prosleđivanje reda i socket-a u nit
        SOCKET* param = (SOCKET*)malloc(sizeof(SOCKET) + sizeof(queue*));
        *param = clientSocket;
        *(queue**)(param + 1) = messQueue;

        HANDLE thread = CreateThread(NULL, 0, clientHandler, param, 0, NULL);
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