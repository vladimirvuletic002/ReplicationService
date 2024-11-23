#include "../Server/common.h"

#define ORIGINAL_PORT 8080
#define SERVER_IP_ADDRESS "127.0.0.1"
#define MAX_MEASUREMENTS 100

bool InitializeWindowsSockets();
void inputMeasurement(Measurement* m);
void print_menu();

int main() {

    SOCKET connectSocket = INVALID_SOCKET;
    Measurement measurement;
    int iResult;
    int choice;


    if (InitializeWindowsSockets() == false)
    {
        // Logovanje je odradjeno u InitializeWindowsSockets() f-ji
        return 1;
    }

    connectSocket = socket(AF_INET,
        SOCK_STREAM,
        IPPROTO_TCP);

    if (connectSocket == INVALID_SOCKET)
    {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // create and initialize address structure
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(SERVER_IP_ADDRESS);
    serverAddress.sin_port = htons(ORIGINAL_PORT);
    // connect to server specified in serverAddress and socket connectSocket
    if (connect(connectSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
    {
        printf("Unable to connect to server.\n");
        closesocket(connectSocket);
        WSACleanup();
    }

    while(1){
        print_menu();
        scanf_s("%d", &choice);

        switch (choice) {
            case 1:
                inputMeasurement(&measurement);

                // Send an prepared message with null terminator included
                iResult = send(connectSocket, (const char*)&measurement, sizeof(Measurement), 0);

                if (iResult == SOCKET_ERROR)
                {
                    printf("send failed with error: %d\n", WSAGetLastError());
                    closesocket(connectSocket);
                    WSACleanup();
                    return 1;
                }
                printf("Merenje uredjaja sa ID-em: %d poslato serveru!\n\n", measurement.deviceId);
                break;
            case 2:
                printf("Zatvaranje klijentske aplikacije.\n");
                closesocket(connectSocket);
                WSACleanup();
                return 0;
            default:
                printf("Nevazeci izbor. Pokusajte ponovo.\n");
                break;
        }
    }

    // cleanup
    //closesocket(connectSocket);
    //WSACleanup();

	return 0;
}

void print_menu() {
    printf("---------UI za unos merenja---------\n");
    printf("1 - Unos novog merenja\n");
    printf("2 - Izlaz iz aplikacije\n");
    printf("Izaberi opciju: ");
}

void inputMeasurement(Measurement* m) {
    printf("\n----------Unesi novo merenje----------\n");
    printf("ID uredjaja: "); 
    scanf_s("%d", &m->deviceId);
    printf("Jacina struje [A]: ");
    scanf_s("%f", &m->current);
    printf("Napon [V]: ");
    scanf_s("%f", &m->voltage);
    printf("Snaga [kW]: ");
    scanf_s("%f", &m->power);
    printf("--------------------------------------\n");
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