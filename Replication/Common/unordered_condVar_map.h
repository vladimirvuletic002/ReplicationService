#ifndef UNODRDERED_CONDVAR_MAP_H
#define UNORDERED_CONDVAR_MAP_H
//#include "Queue.h" // Ukljucujemo definiciju `queue`
#include <stdlib.h>
#include <stdio.h>
#include <mutex>
#include <condition_variable>
#include "Common.h"
#define INITIAL_CAPACITY 16
#define LOAD_FACTOR 0.75


// Funkcije
void init_condVar_map(unordered_condVar_map* map);
void free_condVar_map(unordered_condVar_map* map);

int insert_condVar_to_map(unordered_condVar_map* map, int key);
std::condition_variable* get_condVar_for_key(unordered_condVar_map* map, int key);

int contains_key_condVar(unordered_condVar_map* map, int key);


#endif
#pragma once
