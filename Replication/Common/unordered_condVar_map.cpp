#include "unordered_condVar_map.h"

// Hash funkcija
static size_t hash(int key, size_t capacity) {
    return (size_t)(key % (int)capacity);
}


void init_condVar_map(unordered_condVar_map* map) {
    map->capacity = INITIAL_CAPACITY;
    map->size = 0;
    map->buckets = (KeyValuePairCondVar**)calloc(map->capacity, sizeof(KeyValuePairMtx*));
    if (!map->buckets) {
        fprintf(stderr, "Memory allocation failed!\n");
        exit(EXIT_FAILURE);
    }
}


void free_condVar_map(unordered_condVar_map* map) {
    for (size_t i = 0; i < map->capacity; ++i) {
        KeyValuePairCondVar* current = map->buckets[i];
        while (current) {
            KeyValuePairCondVar* temp = current;
            current = current->next;
            delete temp; // Oslobodi std::mutex automatski
        }
    }
    free(map->buckets);
    map->buckets = NULL;
    map->capacity = 0;
    map->size = 0;
}


int contains_key_condVar(unordered_condVar_map* map, int key) {
    size_t index = hash(key, map->capacity);
    KeyValuePairCondVar* current = map->buckets[index];
    while (current) {
        if (current->key == key) {
            return 1;
        }
        current = current->next;
    }
    return 0;
}

static KeyValuePairCondVar* create_pair(int key) {
    KeyValuePairCondVar* new_pair = (KeyValuePairCondVar*)malloc(sizeof(KeyValuePairCondVar));
    if (!new_pair) {
        fprintf(stderr, "Memory allocation failed!\n");
        return nullptr;
    }

    new_pair->key = key;

    // Inicijalizacija `std::condition_variable` pomocu default konstruktora
    new (&new_pair->value) std::condition_variable();

    new_pair->next = nullptr;
    return new_pair;
}

int insert_condVar_to_map(unordered_condVar_map* map, int key) {
    size_t index = hash(key, map->capacity);

    KeyValuePairCondVar* current = map->buckets[index];
    while (current) {
        if (current->key == key) {
            // Kljuc vec postoji;
            return 1;
        }
        current = current->next;
    }

    // Kljuc ne postoji; kreiraj novi par
    KeyValuePairCondVar* new_pair = create_pair(key);
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


std::condition_variable* get_condVar_for_key(unordered_condVar_map* map, int key) {
    size_t index = hash(key, map->capacity);
    KeyValuePairCondVar* current = map->buckets[index];
    while (current) {
        if (current->key == key) {
            return &current->value;
        }
        current = current->next;
    }
    return NULL; // Kljuc nije pronadjen
}