#ifndef UNORDERED_MAP_H
#define UNORDERED_MAP_H

//#include "Queue.h" // Ukljucujemo definiciju `queue`
#include "Common.h"
#include <stdlib.h>
#include <stdio.h>

#define INITIAL_CAPACITY 16
#define LOAD_FACTOR 0.75

typedef struct KeyValuePair {
    int key;                // Kljuc je tipa `int`
    queue value;            // Vrednost je tipa `queue`
    struct KeyValuePair* next; // Za ulancavanje
} KeyValuePair;

typedef struct {
    KeyValuePair** buckets; // Niz pokazivaca na ulancane liste
    size_t capacity;        // Kapacitet tabele
    size_t size;            // Trenutni broj elemenata
} unordered_map;

// Funkcije
void init_map(unordered_map* map);
void free_map(unordered_map* map);

int insert_queue_to_map(unordered_map* map, int key, queue *value);
queue* get_queue_for_key(unordered_map* map, int key);
int erase(unordered_map* map, int key);
int contains_key(unordered_map* map, int key);

void print_map(unordered_map* map); // Za debug

#endif // UNORDERED_MAP_H
