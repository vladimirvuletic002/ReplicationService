// CommonStatic.cpp : Defines the functions for the static library.
//

#include "pch.h"
#include "framework.h"

// TODO: This is an example of a library function


void init_queue(queue* q) {
	q->head = NULL;
	q->tail = NULL;
	q->size = 0;
}

bool is_queue_empty(queue* q) {
	if (q->size == 0) return true;
	return false;
}


bool enqueue(queue* q, Measurement el) {
	node* newnode = (node*)malloc(sizeof(node));
	if (newnode == NULL) {
		return false;
	}

	newnode->data = el;
	newnode->next = NULL;

	if (q->tail != NULL) {
		q->tail->next = newnode;
	}
	q->tail = newnode;

	if (q->head == NULL) {
		q->head = newnode;
	}

	q->size++;

	return true;
}

int dequeue(queue* q, Measurement* el) {
	if (q->head == NULL) return QUEUE_EMPTY;
	node* tmp = q->head;
	*el = tmp->data;
	q->head = tmp->next;

	if (q->head == NULL) {
		q->tail = NULL;
	}

	free(tmp);
	q->size--;

	return 0;
}

void print_queue(queue* q) {
	if (is_queue_empty(q)) {
		printf("Red poruka je prazan!\n");
		return;
	}

	node* curr = q->head;
	printf("Merenja: \n");
	while (curr) {
		Measurement m = curr->data;
		printf("ID uredjaja: %d, Jacina struje: %.2f [A], Napon: %.2f [V], Snaga: %.2f [W]\n", m.deviceId, m.current, m.voltage, m.power);
		curr = curr->next;
	}
}

void free_queue(queue* q) {
	node* curr = q->head;
	while (curr) {
		node* tmp = q->head;
		curr = curr->next;
		free(tmp);
	}
	q->head = NULL;
	q->tail = NULL;
	q->size = 0;
}





