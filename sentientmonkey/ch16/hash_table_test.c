#include "hash_table.h"
#include <stdio.h>

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
    hash_table_resize(h, 20);
    hash_table_print(h);
    hash_table_delete(h);
    return 0;
}
