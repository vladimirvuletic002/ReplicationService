#include "req_queue.h"


void init_req_queue(req_queue* queue, int capacity) {
    queue->requests = (SOCKET*)malloc(sizeof(SOCKET) * capacity);
    queue->capacity = capacity;
    queue->front = 0;
    queue->rear = -1;
    queue->size = 0;

}

void enqueue_request(req_queue* queue, SOCKET req) {
    if (queue->size == queue->capacity) {
        //red pun
        return;
    }
    queue->rear = (queue->rear + 1) % queue->capacity;
    queue->requests[queue->rear] = req;
    queue->size++;
}

SOCKET dequeue_request(req_queue* queue) {
    if (queue->size == 0) {
        return INVALID_SOCKET;
    }
    SOCKET request = queue->requests[queue->front];
    queue->front = (queue->front + 1) % queue->capacity;
    queue->size--;
    return request;
}

void free_req_queue(req_queue* queue) {
    free(queue->requests);
}

bool is_req_queue_empty(req_queue* queue) {
    
    return queue->size == 0;

}