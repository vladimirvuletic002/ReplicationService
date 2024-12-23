#include "unordered_map.h"

static size_t hash(int key, size_t capacity){
	return key % capacity;
}

void init_map(unordered_map *map){
	map->capacity = INITIAL_CAPACITY;
	map->size = 0;
	map->buckets = (KeyValuePair**)calloc(map->capacity, sizeof(KeyValuePair*));
}


void free_map(unordered_map* map) {
    for (size_t i = 0; i < map->capacity; ++i) {
        KeyValuePair* current = map->buckets[i];
        while (current) {
            KeyValuePair* temp = current;
            current = current->next;
            free_queue(&temp->value); // Oslobodi red
            free(temp);
        }
    }
    free(map->buckets);
    map->buckets = NULL;
    map->size = 0;
    map->capacity = 0;
}


static KeyValuePair* create_pair(int key, queue* value) {
    KeyValuePair* pair = (KeyValuePair*)malloc(sizeof(KeyValuePair));
    pair->key = key;
    init_queue(&pair->value);          // Inicijalizuj red u paru
    if (value) {                       // Kopiraj podatke ako postoji izvorni red
        node* current = value->head;
        while (current) {
            enqueue(&pair->value, current->data);
            current = current->next;
        }
    }
    pair->next = NULL;
    return pair;
}

// Dodavanje elementa u mapu
int insert_queue_to_map(unordered_map* map, int key, queue* value) {
    size_t index = hash(key, map->capacity);

    KeyValuePair* current = map->buckets[index];
    while (current) {
        if (current->key == key) {
            // Kljuc vec postoji, azuriraj vrednost (oslobodi stari red)
            free_queue(&current->value);
            init_queue(&current->value);
            if (value) {
                node* cur = value->head;
                while (cur) {
                    enqueue(&current->value, cur->data);
                    cur = cur->next;
                }
            }
            return 1; 
        }
        current = current->next;
    }

    // Kljuc ne postoji; dodaj novi element
    KeyValuePair* new_pair = create_pair(key, value);
    new_pair->next = map->buckets[index];
    map->buckets[index] = new_pair;

    map->size++;

    // Proveri da li je potrebno prosiriti mapu
    if ((float)map->size / map->capacity > LOAD_FACTOR) {
        fprintf(stderr, "Warning: Load factor exceeded; resizing not implemented.\n");
    }

    return 0; 
}


queue* get_queue_for_key(unordered_map* map, int key) {
    size_t index = hash(key, map->capacity);

    KeyValuePair* current = map->buckets[index];
    while (current) {
        if (current->key == key) {
            return &current->value;
        }
        current = current->next;
    }

    return NULL; 
}


int erase(unordered_map* map, int key) {
    size_t index = hash(key, map->capacity);

    KeyValuePair* current = map->buckets[index];
    KeyValuePair* prev = NULL;

    while (current) {
        if (current->key == key) {
            if (prev) {
                prev->next = current->next;
            }
            else {
                map->buckets[index] = current->next;
            }
            free_queue(&current->value);
            free(current);
            map->size--;
            return 1; // Element je uspesno obrisan
        }
        prev = current;
        current = current->next;
    }

    return 0; 
}

int contains_key(unordered_map* map, int key) {
    size_t index = hash(key, map->capacity);  
    KeyValuePair* current = map->buckets[index];

    while (current) {
        if (current->key == key) {
            return 1;  
        }
        current = current->next;
    }

    return 0;  
}

// Debug ispis
void print_map(unordered_map* map) {
    for (size_t i = 0; i < map->capacity; ++i) {
        printf("[%zu]: ", i);
        KeyValuePair* current = map->buckets[i];
        while (current) {
            printf("(%d -> ", current->key);
            print_queue(&current->value); // Koristimo prethodno implementiranu funkciju
            printf(") ");
            current = current->next;
        }
        printf("\n");
    }
}



