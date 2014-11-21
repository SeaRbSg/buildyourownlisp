#define DEFAULT_HASH_SIZE 5
#define DEFAULT_LOAD_FACTOR 0.75

typedef struct hash_table hash_table;
typedef struct entry entry;
typedef void (*print_func)(void*);
typedef void* (*copy_func)(void*);
typedef void (*delete_func)(void*);

struct hash_table {
    entry** entries;
    unsigned long size;
    unsigned long capacity;
    print_func print_func;
    copy_func copy_func;
    delete_func delete_func;
};

struct entry {
    char* key;
    void* value;
    entry* next;
};


hash_table* hash_table_new();
void hash_table_register_print(hash_table* h, print_func print_func);
void hash_table_register_copy(hash_table* h, copy_func copy_func);
void hash_table_register_delete(hash_table* h, delete_func delete_func);
void hash_table_delete(hash_table* h);
void hash_table_print(hash_table* h);
void* hash_table_add(hash_table* h, char* key, void* value);
void* hash_table_get(hash_table* h, char* key);
void* hash_table_remove(hash_table* h, char* key);
void hash_table_resize(hash_table* h, unsigned long size);
