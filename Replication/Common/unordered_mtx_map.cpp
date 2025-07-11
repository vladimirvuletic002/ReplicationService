#include "unordered_mtx_map.h"

// Hash funkcija
static size_t hash(int key, size_t capacity) {
    return (size_t)(key % (int)capacity);
}


void init_mtx_map(unordered_mtx_map* map) {
    map->capacity = INITIAL_CAPACITY;
    map->size = 0;
    map->buckets = (KeyValuePairMtx**)calloc(map->capacity, sizeof(KeyValuePairMtx*));
    if (!map->buckets) {
        fprintf(stderr, "Memory allocation failed!\n");
        exit(EXIT_FAILURE);
    }
}


void free_mtx_map(unordered_mtx_map* map) {
    for (size_t i = 0; i < map->capacity; ++i) {
        KeyValuePairMtx* current = map->buckets[i];
        while (current) {
            KeyValuePairMtx* temp = current;
            current = current->next;
            delete temp; // Oslobodi std::mutex automatski
        }
    }
    free(map->buckets);
    map->buckets = NULL;
    map->capacity = 0;
    map->size = 0;
}


int contains_key_mtx(unordered_mtx_map* map, int key) {
    size_t index = hash(key, map->capacity);
    KeyValuePairMtx* current = map->buckets[index];
    while (current) {
        if (current->key == key) {
            return 1; 
        }
        current = current->next;
    }
    return 0; 
}

static KeyValuePairMtx* create_pair(int key) {
    KeyValuePairMtx* new_pair = (KeyValuePairMtx*)malloc(sizeof(KeyValuePairMtx));
    if (!new_pair) {
        fprintf(stderr, "Memory allocation failed!\n");
        return nullptr;
    }

    new_pair->key = key;

    // Inicijalizacija `std::mutex` pomocu default konstruktora
    new (&new_pair->value) std::mutex();

    new_pair->next = nullptr;
    return new_pair;
}

int insert_mtx_to_map(unordered_mtx_map* map, int key) {
    size_t index = hash(key, map->capacity);

    KeyValuePairMtx* current = map->buckets[index];
    while (current) {
        if (current->key == key) {
            // Kljuc vec postoji;
            return 1;
        }
        current = current->next;
    }

    // Kljuc ne postoji; kreiraj novi par
    KeyValuePairMtx* new_pair = create_pair(key);
    if (!new_pair) {
        return -1; // Greska pri alokaciji
    }

    new_pair->next = map->buckets[index];
    map->buckets[index] = new_pair;

    map->size++;

    // Proveri da li je potrebno prosiriti mapu
    if ((float)map->size / map->capacity > LOAD_FACTOR) {
        fprintf(stderr, "Warning: Load factor exceeded; resizing not implemented.\n");
    }

    return 0;
}


std::mutex* get_mtx_for_key(unordered_mtx_map* map, int key) {
    size_t index = hash(key, map->capacity);
    KeyValuePairMtx* current = map->buckets[index];
    while (current) {
        if (current->key == key) {
            return &current->value;
        }
        current = current->next;
    }
    return NULL; // Kljuc nije pronadjen
}