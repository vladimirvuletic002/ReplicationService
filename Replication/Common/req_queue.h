#ifndef REQ_QUEUE_H
#define REQ_QUEUE_H
#include <winsock2.h>
#pragma comment (lib, "Ws2_32.lib")

typedef struct req_queue {
    SOCKET* requests;        // Niz zahteva (socket-a)
    int capacity;         // Kapacitet reda
    int front;            // Pokazivac na prvi el. u redu
    int rear;             // Pokazivac na poslednji el. u redu
    int size;             // Trenutni br elemenata u redu
} req_queue;


void init_req_queue(req_queue* queue, int capacity);

bool is_req_queue_empty(req_queue* queue);

void enqueue_request(req_queue* queue, SOCKET req);

SOCKET dequeue_request(req_queue* queue);

void free_req_queue(req_queue* queue);




#endif

