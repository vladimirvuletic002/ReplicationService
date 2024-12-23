#pragma once
#define UNORDERED_MAP_H
//#include "Queue.h" // Ukljucujemo definiciju `queue`
#include "Queue.h"
#include <stdlib.h>
#include <stdio.h>

#define INITIAL_CAPACITY 16
#define LOAD_FACTOR 0.75


// Funkcije
void init_map(unordered_map* map);
void free_map(unordered_map* map);

int insert_queue_to_map(unordered_map* map, int key, queue *value);
queue* get_queue_for_key(unordered_map* map, int key);
int erase(unordered_map* map, int key);
int contains_key(unordered_map* map, int key);

void print_map(unordered_map* map); // Za debugs

// UNORDERED_MAP_H
