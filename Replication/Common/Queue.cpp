#include "Queue.h"
#include "unordered_map.h"




void init_queue(queue* q) {
	q->head = NULL;
	q->tail = NULL;
	q->size = 0;
}

bool is_queue_empty(queue* q) {
	if (q == NULL) {
		return true;
	}
	
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
	if (q == NULL) return SKIP_QUEUE;
	
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

bool free_deq(queue* q) {

	if (q->head == NULL)return false;

	node* tmp = q->head;
	node* tmp2 = nullptr;

	if (tmp->next != nullptr) {
		
		while (tmp != q->tail) {
			tmp2 = tmp;
			tmp = tmp->next;
		}
		tmp2->next = nullptr;
		free(tmp);

	}
	else {
		free_queue(q);
	}
	q->size--;

	return true;
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
		printMeasurement(&m);
		curr = curr->next;
	}
}

void free_queue(queue* q) {
	if (q->head == nullptr)return;
	node* curr = q->head;
	while (curr) {
		node* tmp = curr;
		curr = curr->next;
		free(tmp);
	}
	q->head = NULL;
	q->tail = NULL;
	q->size = 0;
}





