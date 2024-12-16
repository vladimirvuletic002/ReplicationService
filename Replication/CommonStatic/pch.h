// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

//#include "Common.h"

// add headers that you want to pre-compile here
#include "framework.h"

#ifndef QUEUE_H
#define QUEUE_H


#define QUEUE_EMPTY INT_MIN
#define MAX_MESSAGE_LENGTH 1024

typedef struct node {
    Measurement data;
    struct node* next;
} node;

typedef struct {
    node* head;  //Pok. na prvi el. reda
    node* tail;  //Pok. na poslednji el. reda
    int size;   //broj el. u redu
} queue;



void init_queue(queue* q);

bool is_queue_empty(queue* q);

bool enqueue(queue* q, Measurement el);

int dequeue(queue* q, Measurement* m);

void free_queue(queue* q);

void print_queue(queue* q);



#endif
#endif //PCH_H
