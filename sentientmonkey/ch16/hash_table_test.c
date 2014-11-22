#include "hash_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void int_print(void* value) {
    int* i = (int*)value;
    printf("%i", *i);
}

void* int_copy(void* value) {
    int* r = malloc(sizeof(int));
    memcpy(r, value, sizeof(int));
    return r;
}

void int_delete(void* value) {
    free(value);
}

int main() {
    hash_table* h = hash_table_new();
    hash_table_register_print(h, int_print);
    hash_table_register_copy(h, int_copy);
    hash_table_register_delete(h, int_delete);
    hash_table_print(h);
    char buff[3];
    int ints[20];
    for(int i=0; i < 20; i++) {
        ints[i] = i;
        sprintf(buff, "%i", i);
        hash_table_add(h, buff, &(ints[i]));
    }
    hash_table_print(h);
    hash_table_get(h, "1");
    hash_table_remove(h, "1");
    hash_table_print(h);
    hash_table_delete(h);
    return 0;
}
