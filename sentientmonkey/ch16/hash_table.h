typedef struct hash_table hash_table;
typedef struct entry entry;

struct hash_table {
    entry** entries;
    unsigned long size;
};

struct entry {
    char* key;
    void* value;
    entry* next;
};

hash_table* hash_table_new(unsigned long size);
void hash_table_delete(hash_table* h);
void hash_table_print(hash_table* h);
void* hash_table_add(hash_table* h, char* key, void* value);
void* hash_table_get(hash_table* h, char* key);
void* hash_table_remove(hash_table* h, char* key);
void hash_table_resize(hash_table* h, unsigned long size);
