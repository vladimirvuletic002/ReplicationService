#include "../Common/Common.h"
#include<iostream>

#include <random>

#define CURRENT_PORT 8081
#define VOLTAGE_PORT 8082
#define POWER_PORT 8083
#define SERVER_IP_ADDRESS "127.0.0.1"
#define MAX_MEASUREMENTS 100

bool InitializeWindowsSockets();
void inputMeasurement(Measurement* m, int selectedPort);
int choose_device(Measurement* m);
void print_menu();
void massSend(SOCKET s,Measurement *m,int ms);
void getBackup(SOCKET s,Measurement& m);
bool print = false;


int main() {


    SOCKET connectSocket = INVALID_SOCKET;
    Measurement measurement;
    int iResult;
    int choice;
    int selPort;

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

    selPort = choose_device(&measurement);
   
    ///////////////

    // create and initialize address structure
   
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(SERVER_IP_ADDRESS);
    serverAddress.sin_port = htons(selPort);

    // connect to server specified in serverAddress and socket connectSocket
    if (connect(connectSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
    {
        printf("Unable to connect to server.\n");
        closesocket(connectSocket);
        WSACleanup();
    }

   
    while (1) {
        print_menu();
        scanf_s("%d", &choice);
        

        switch (choice) {
        case 1:
           
            inputMeasurement(&measurement, selPort);
            // Send an prepared message with null terminator included
            iResult = send(connectSocket, (const char*)&measurement, sizeof(Measurement), 0);

            if (iResult == SOCKET_ERROR)
            {
                printf("send failed with error: %d\n", WSAGetLastError());
                closesocket(connectSocket);
                WSACleanup();
                return 1;
            }
            printf("\nMeasurement with ID: %d sent to server!\n\n", measurement.deviceId);
            break;
        case 2: 
            massSend(connectSocket,&measurement,5000);
            break;
        case 3:
            massSend(connectSocket, &measurement, 10000);
            break;
        case 4:
            massSend(connectSocket, &measurement, 100000);
            break;
        case 5:
            massSend(connectSocket, &measurement, 1000000);
            break;
        case 6:
            getBackup(connectSocket, measurement);
            break;

        case 7:
            if (print == true) {
                printf("\n\t\t\t\t--------------------------\n\t\t\t\tMore feedback turned OFF\n\t\t\t\t--------------------------\n");
                print = false;
            }
            else {

                print = true;
                printf("\n\t\t\t\t--------------------------\n\t\t\t\tMore feedback turned ON\n\t\t\t\t--------------------------\n");
            }
            break;

        case 8:
            printf("\nClosing client application.\n");
            closesocket(connectSocket);
            WSACleanup();
            return 0;
           
        default:
            printf("\nInvalid option. Try again!\n");
            break;
        }
    }

    // cleanup
    closesocket(connectSocket);
     WSACleanup();

    return 0;
}




void print_menu() {
    printf("\n---------UI za izbor opcije---------\n");
    printf("1 - Unos novog merenja\n");
    printf("2 - Posalji 5.000 nasumicnih merenja\n");
    printf("3 - Posalji 10.000 nasumicnih merenja\n");
    printf("4 - Posalji 100.000 nasumicnih merenja\n");
    printf("5 - Posalji 1.000.000 nasumicnih merenja\n");
    printf("6 - Zatrazi backup\n");
    printf("7 - Prikazi vise informacija\n");
    printf("8 - Napusti Aplikaciju\n");
    printf("Izaberi opciju: ");
}



int choose_device(Measurement* m) {
    int choice = -1;
    int selected;
    do {
        printf("---------Izbor Uredjaja---------\n");
        printf("1 - Ampermetar [A]\n");
        printf("2 - Voltmetar [V]\n");
        printf("3 - Vatmetar [kW]\n");
        printf("Izaberi tip merenja: ");
        if (scanf_s("%d", &choice) != 1) {  // Check if input is not valid
            printf("Invalid input. Please enter a valid number.\n");
            while (getchar() != '\n');  // Discard characters until a newline is found
            choice = -1;
        }
    } while (choice < 1 || choice >3);


    // Postavljanje porta na osnovu korisnickog izbora
    switch (choice) {
    case 1:
        selected = CURRENT_PORT;
        m->deviceId = 1;
        break;
    case 2:
        selected = VOLTAGE_PORT;
        m->deviceId = 2;
        break;
    case 3:
        selected = POWER_PORT;
        m->deviceId = 3;
        break;
    default:
        printf("Nevazeci izbor. Zatvaranje aplikacije.\n");
        WSACleanup();
        return 0;
    }
    return selected;
}


void inputMeasurement(Measurement* m, int selectedPort) {
    float val = -1;


    do {

        printf("\n----------Unesi novo merenje----------\n");

        switch (selectedPort) {
        case CURRENT_PORT:
            printf("Jacina struje [A]: ");
            if (scanf_s("%f", &val) != 1) {
                printf("Unos nije validan. Pokusajte ponovo.\n");
                while (getchar() != '\n');
                val = -1;
                continue;
            }
            m->value = val;
            m->type = CURRENT;
            break;
        case VOLTAGE_PORT:
            printf("Napon [V]: ");
            if (scanf_s("%f", &val) != 1) {
                printf("Unos nije validan. Pokusajte ponovo.\n");
                while (getchar() != '\n');
                val = -1;
                continue;
            }
            m->value = val;
            m->type = VOLTAGE;
            break;
        case POWER_PORT:
            printf("Snaga [kW]: ");
            if (scanf_s("%f", &val) != 1) {
                printf("Unos nije validan. Pokusajte ponovo.\n");
                while (getchar() != '\n');
                val = -1;
                continue;
            }
            m->value = val;
            m->type = POWER;
            break;
        }

        

    } while (val < 0);
    
    
    //printf("ID uredjaja: "); 
    //scanf_s("%d", &m->deviceId);

    

    time_t current_time = time(NULL);
    m->timeStamp = *localtime(&current_time);

    printf("--------------------------------------\n");
}


void makeSocketNonBlocking(SOCKET socket) {
    u_long mode = 1; // 1 to enable non-blocking mode
    ioctlsocket(socket, FIONBIO, &mode);
}

void getBackup(SOCKET connectSocket, Measurement& m) {
    m.purpose = 99;
    std::atomic<bool> stopFlag;
    std::atomic<bool> stop = false;
    int i = 0;
    int iResult = send(connectSocket, (const char*)&m, sizeof(Measurement), 0);

    if (iResult == SOCKET_ERROR)
    {
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(connectSocket);
        WSACleanup();
        return;
    }
    else {
        makeSocketNonBlocking(connectSocket);
        while (!stop) {
          
            if (m.purpose == END_OF_QUEUE) { break; }
           int f= nonBlockingReceive(stopFlag, connectSocket, m, stop, print);
           if (f != 2) {
               i++;
               if (i < 100) {
                   printf("\n\t\t\t\tMessage received : % d", i);
               }
               else if (i == 100) {
                   printf("\n\t\t\t\tWait for completion...");
               }
               
           }
        }
        printf("\n\n\t\t\t\tDone.\t\tMessage count: %d",i);
        u_long mode = 0; // 0 for blocking mode

        ioctlsocket(connectSocket, FIONBIO, &mode);

    }
}


void massSend(SOCKET connectSocket, Measurement* m, int messNum) {


    int iResult;

    std::uniform_real_distribution<float> distribution(10, 1000);
    if (messNum > 1000) {
        m->purpose = 8;
    }

    for (int i = 0; i < messNum; i++) {

        std::random_device rd;
        std::mt19937 generator(rd()); // Mersenne Twister RNG

        // Define the distribution range
        std::uniform_real_distribution<float> distribution(10, 1000);

        float res = distribution(generator);

        float randomFloat = distribution(generator);
        std::cout << "Random measure: " << randomFloat << std::endl;


        m->value = randomFloat;
        //m->type = CURRENT;
        time_t current_time = time(NULL);
        m->timeStamp = *localtime(&current_time);

        iResult = send(connectSocket, (const char*)m, sizeof(Measurement), 0);

        if (iResult == SOCKET_ERROR)
        {
            printf("send failed with error: %d\n", WSAGetLastError());
            closesocket(connectSocket);
            WSACleanup();
            return;
        }


    }

    std::cout << "\n\n\n\t\t\tNumber of random measurements sent: " << messNum << std::endl;

}





