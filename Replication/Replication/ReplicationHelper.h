
#include "../Common/unordered_map.h"
#include "../Common/req_queue.h"
#include "../Common/unordered_mtx_map.h"
#define SERVER_IP_ADDRESS "127.0.0.2"
#define COPY_SERVER_IP_ADDRESS "127.0.0.3"
#define NUMBER_OF_POSSIBLE_IDS 3

typedef struct Params {

	SOCKET serverSocket;
	int id;

}Params;




void printfP(const char* text, Measurement* m, std::atomic<int> totalMessages, std::atomic<int> totalBackuo);
void ReceiveAndFilterMessages(std::atomic<bool>& stopFlag, Measurement& m, SOCKET& backupRequest,std::atomic<int>& totalBackup,
bool& stop, int& localCounter, unordered_map &messageQueue, std::mutex& mtx);
//void ReceiveAndFilterMessages(std::atomic<bool>& stopFlag, Measurement& m, SOCKET& backupRequest, int& counter,bool& stop, int& localCounter, queue& backupTempQeueu);