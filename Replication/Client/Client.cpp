#include "../Common/Common.h"
#include "../Common/unordered_map.h"
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
    printf("\n---------UI menu---------\n");
    printf("1 - Enter a measurement\n");
    printf("2 - Send 5.000 random messages\n");
    printf("3 - Send 10.000 random messages\n");
    printf("4 - Send 100.000 random messages\n");
    printf("5 - Send 1.000.000 random messages\n");
    printf("6 - Request Backup\n");
    printf("7 - Print more information\n");
    printf("8 - Exit\n");
    printf("Choose an option: ");
}



int choose_device(Measurement* m) {
    int choice = -1;
    int selected;
    do {
        printf("---------Devices---------\n");
        printf("1 - Ampermeter [A]\n");
        printf("2 - Voltmeter [V]\n");
        printf("3 - Wattmeter [kW]\n");
        printf("Choose>>  ");
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
        printf("Invalid input. Closing the application.\n");
        WSACleanup();
        return 0;
    }
    return selected;
}


void inputMeasurement(Measurement* m, int selectedPort) {
    float val = -1;


    do {

        printf("\n----------Enter new measurement----------\n");

        switch (selectedPort) {
        case CURRENT_PORT:
            printf("Electric current [A]: ");
            if (scanf_s("%f", &val) != 1) {
                printf("Invalid input. Try Again.\n");
                while (getchar() != '\n');
                val = -1;
                continue;
            }
            m->value = val;
            m->type = CURRENT;
            break;
        case VOLTAGE_PORT:
            printf("Voltage [V]: ");
            if (scanf_s("%f", &val) != 1) {
                printf("Invalid input. Try Again.\n");
                while (getchar() != '\n');
                val = -1;
                continue;
            }
            m->value = val;
            m->type = VOLTAGE;
            break;
        case POWER_PORT:
            printf("Power [kW]: ");
            if (scanf_s("%f", &val) != 1) {
                printf("Invalid input. Try Again.\n");
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

    

    //time_t current_time = time(NULL);
    //m->timeStamp = *localtime(&current_time);
    time_t now = time(NULL);
    m->epochTime = now;

    printf("--------------------------------------\n");
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
          
            
           int f= nonBlockingReceive(stopFlag, connectSocket, m, stop, print);
           
           if (m.purpose == END_OF_QUEUE) { break; }
           
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
        //m->timeStamp = *localtime(&current_time);
        m->epochTime = current_time;

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





