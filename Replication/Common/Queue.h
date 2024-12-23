#ifndef QUEUE_H
#define QUEUE_H

#include "Common.h"

#define QUEUE_EMPTY INT_MIN
#define MAX_MESSAGE_LENGTH 1024




bool free_deq(queue* q);

void init_queue(queue* q);

bool is_queue_empty(queue* q);

bool enqueue(queue* q, Measurement el);

int dequeue(queue* q, Measurement* m);

void free_queue(queue* q);

void print_queue(queue* q);



#endif