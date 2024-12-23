#ifndef UNODRDERED_MTX_MAP_H
#define UNORDERED_MTX_MAP_H
//#include "Queue.h" // Ukljucujemo definiciju `queue`
#include <stdlib.h>
#include <stdio.h>
#include <mutex>
#include "Common.h"
#define INITIAL_CAPACITY 16
#define LOAD_FACTOR 0.75


// Funkcije
void init_mtx_map(unordered_mtx_map* map);
void free_mtx_map(unordered_mtx_map* map);

int insert_mtx_to_map(unordered_mtx_map* map, int key);
std::mutex* get_mtx_for_key(unordered_mtx_map* map, int key);
//int erase_mtx(unordered_mtx_map* map, int key);
int contains_key_mtx(unordered_mtx_map* map, int key);

//void print_map(unordered_mtx_map* map); // Za debugs

#endif
