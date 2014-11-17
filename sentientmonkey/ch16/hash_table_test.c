#include "hash_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    int a = 1;
    int b = 2;
    hash_table* h = hash_table_new(10);
    hash_table_print(h);
    hash_table_add(h, "foo", &a);
    int* r = hash_table_get(h, "foo");
    printf("foo is %i\n", *r);
    hash_table_print(h);
    hash_table_add(h, "bar", &b);
    r = hash_table_get(h, "bar");
    printf("bar is %i\n", *r);
    hash_table_print(h);
    printf("adding foo again\n");
    hash_table_add(h, "foo", &b);
    hash_table_print(h);
    printf("foo is %i\n", *r);
    printf("removing foo\n");
    r = hash_table_remove(h, "foo");
    hash_table_print(h);
    printf("r is %i\n", *r);
    char buff[3];
    int ints[20];
    for(int i=0; i < 20; i++) {
        ints[i] = i;
        sprintf(buff, "%i", i);
        hash_table_add(h, buff, &(ints[i]));
    }
    hash_table_print(h);
    hash_table_delete(h);
    return 0;
}
