#include "hash_table.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* djb2 */
unsigned long hash(char* key) {
    unsigned long hash = 5381;
    int c;

    while ((c = *key++)) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash;
}

entry* entry_new(char* key, void* value) {
    entry* e = malloc(sizeof(entry));
    e->key = strdup(key);
    e->value = value;
    e->next = NULL;
    return e;
}

// returns value so caller can delete
entry* entry_delete(entry* e) {
    entry* value = e->value;
    free(e->key);
    free(e);
    return value;
}

hash_table* hash_table_new(unsigned long size) {
    hash_table* h = malloc(sizeof(hash_table));
    h->entries = malloc(sizeof(entry)*size);
    h->size = size;
    for(unsigned int i=0; i < size; i++) {
        h->entries[i] = NULL;
    }
    return h;
}

void* hash_table_get(hash_table* h, char* key) {
    unsigned long bucket = hash(key) % h->size;
    entry* e = h->entries[bucket];
    while (e != NULL) {
        if (strcmp(key, e->key) == 0) {
            return e->value;
        }
        e = e->next;
    }
    return NULL;
}

void* hash_table_add(hash_table* h, char* key, void* value) {
    unsigned long bucket = hash(key) % h->size;
    entry* e = entry_new(key, value);
    void* r = NULL;

    entry* parent = h->entries[bucket];
    entry* previous = NULL;
    if (parent == NULL) {
        h->entries[bucket] = e;
    }
    while (parent != NULL) {
        if (strcmp(parent->key, key) == 0) {
            e->next = parent->next;
            r = entry_delete(parent);
            if (previous == NULL) {
                h->entries[bucket] = e;
            } else {
                previous->next = e;
            }
            break;
        }
        if (parent->next == NULL) {
            parent->next = e;
            parent = e;
        }
        previous = parent;
        parent = parent->next;
    }
    return r;
}

// returns value for caller to delete
void* hash_table_remove(hash_table* h, char* key) {
    unsigned long bucket = hash(key) % h->size;
    void* r = NULL;

    entry* e = h->entries[bucket];
    entry* previous = NULL;
    while (e != NULL) {
        if (strcmp(key, e->key) == 0) {
            entry* next = e->next;
            r = entry_delete(e);
            if (previous == NULL) {
                h->entries[bucket] = next;
            } else {
                previous->next = next;
            }
        } else {
            previous = e;
            e = e->next;
        }
    }
    return r;
}

void hash_table_print(hash_table* h) {
    if (h == NULL) {
        return;
    }
    printf("{");
    entry* last = NULL;
    for (unsigned long i=0; i < h->size; i++) {
        entry* e = h->entries[i];
        while (e != NULL) {
            if (last != NULL) {
                printf(", ");
            }
            printf("%s: %p", e->key, e->value);
            last = e;
            e = e->next;
        }
    }
    printf("} size: %li\n", h->size);
}

void hash_table_delete(hash_table* h) {
    entry* e = NULL;
    entry* next = NULL;
    for (unsigned long i=0; i < h->size; i++) {
        e = h->entries[i];
        while (e != NULL) {
            next = e->next;
            entry_delete(e);
            e = next;
        }
    }
    free(h->entries);
    free(h);
}

void hash_table_resize(hash_table* h, unsigned long size) {
    entry** new_entries = malloc(sizeof(entry)*size);
    for (unsigned long i=0; i < size; i++) {
        new_entries[i] = NULL;
    }

    entry* e = NULL;
    entry* p = NULL;
    unsigned long bucket;

    for (unsigned long i=0; i < h->size; i++) {
        e = h->entries[i];
        while (e != NULL) {
            bucket = hash(e->key) % size;
            p = new_entries[bucket];
            if (p == NULL) {
                new_entries[bucket] = e;
            } else {
                while (p->next != NULL) {
                    p = p->next;
                }
                p->next = e;
            }
            p = e;
            e = e->next;
            p->next = NULL;
        }
    }

    free(h->entries);
    h->size = size;
    h->entries = new_entries;
}
