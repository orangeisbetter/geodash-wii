#include "hash_table.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <gccore.h>

#include "prime.h"
#include "../vendor/xml.h"
#include "../parse/plist.h"

#define HT_PRIME_1 151
#define HT_PRIME_2 163

#define HT_INITIAL_BASE_SIZE 50

static ht_hash_table* table;

static ht_item HT_DELETED_ITEM = { NULL, { 0 } };

static void _ht_del_hash_table(ht_hash_table* ht);
static void _ht_insert(ht_hash_table* ht, const char* key, texture_info value);
static texture_info* _ht_search(ht_hash_table* ht, const char* key);
static void _ht_delete(ht_hash_table* ht, const char* key);

static ht_item* ht_new_item(const char* k, texture_info v) {
    ht_item* i = malloc(sizeof(ht_item));
    i->key = strdup(k);
    i->value = v;
    return i;
}

static ht_hash_table* ht_new_sized(const int base_size) {
    ht_hash_table* ht = malloc(sizeof(ht_hash_table));
    ht->base_size = base_size;
    ht->size = next_prime(ht->base_size);
    ht->count = 0;
    ht->items = calloc((size_t)ht->size, sizeof(ht_item*));
    return ht;
}

static ht_hash_table* ht_new() {
    return ht_new_sized(HT_INITIAL_BASE_SIZE);
}

void ht_init() {
    table = ht_new();
}

static void ht_del_item(ht_item* i) {
    free(i->key);
    // free(i->value);
    free(i);
}

static void _ht_del_hash_table(ht_hash_table* ht) {
    for (int i = 0; i < ht->size; i++) {
        ht_item* item = ht->items[i];
        if (item != NULL) {
            ht_del_item(item);
        }
    }
    free(ht->items);
    free(ht);
}

static unsigned int ht_hash(const char* s, const int a, const int m) {
    unsigned int hash = 0;
    const int len_s = strlen(s);
    unsigned long multiplier = 1;
    for (int i = 0; i < len_s; i++) {
        hash += multiplier * (unsigned char)s[i];
        multiplier *= a;
    }
    // hash = hash % m;
    return (int)hash;
}

static int ht_get_hash(const char* s, const int num_buckets, const int attempt) {
    const unsigned int hash_a = ht_hash(s, HT_PRIME_1, num_buckets);
    if (attempt == 0) {
        return hash_a % num_buckets;
    } else {
        const unsigned int hash_b = ht_hash(s, HT_PRIME_2, num_buckets);
        return (hash_a + attempt * (hash_b + 1)) % num_buckets;
    }
}

static void ht_resize(ht_hash_table* ht, const int base_size) {
    if (base_size < HT_INITIAL_BASE_SIZE) {
        return;
    }
    ht_hash_table* new_ht = ht_new_sized(base_size);
    for (int i = 0; i < ht->size; i++) {
        ht_item* item = ht->items[i];
        if (item != NULL && item != &HT_DELETED_ITEM) {
            _ht_insert(new_ht, item->key, item->value);
        }
    }

    ht->base_size = new_ht->base_size;
    ht->count = new_ht->count;

    const int tmp_size = ht->size;
    ht->size = new_ht->size;
    new_ht->size = tmp_size;

    ht_item** tmp_items = ht->items;
    ht->items = new_ht->items;
    new_ht->items = tmp_items;

    _ht_del_hash_table(new_ht);
}

static void ht_resize_up(ht_hash_table* ht) {
    const int new_size = ht->base_size * 2;
    ht_resize(ht, new_size);
}

static void ht_resize_down(ht_hash_table* ht) {
    const int new_size = ht->base_size / 2;
    ht_resize(ht, new_size);
}

static void _ht_insert(ht_hash_table* ht, const char* key, texture_info value) {
    const int load = ht->count * 100 / ht->size;
    if (load > 70) {
        ht_resize_up(ht);
    }
    ht_item* item = ht_new_item(key, value);
    int index = ht_get_hash(item->key, ht->size, 0);
    ht_item* cur_item = ht->items[index];
    int i = 1;
    while (cur_item != NULL) {
        if (cur_item != &HT_DELETED_ITEM) {
            if (strcmp(cur_item->key, key) == 0) {
                ht_del_item(cur_item);
                ht->items[index] = item;
                return;
            }
        }
        index = ht_get_hash(item->key, ht->size, i);
        cur_item = ht->items[index];
        i++;
    }
    ht->items[index] = item;
    ht->count++;
}

static texture_info* _ht_search(ht_hash_table* ht, const char* key) {
    if (key == NULL) {
        return NULL;
    }
    int index = ht_get_hash(key, ht->size, 0);
    ht_item* item = ht->items[index];
    int i = 1;
    while (item != NULL) {
        if (item != &HT_DELETED_ITEM) {
            if (strcmp(item->key, key) == 0) {
                return &item->value;
            }
        }
        index = ht_get_hash(key, ht->size, i);
        item = ht->items[index];
        i++;
    }
    return NULL;
}

static void _ht_delete(ht_hash_table* ht, const char* key) {
    int index = ht_get_hash(key, ht->size, 0);
    ht_item* item = ht->items[index];
    int i = 1;
    while (item != NULL) {
        if (item != &HT_DELETED_ITEM) {
            if (strcmp(item->key, key) == 0) {
                ht_del_item(item);
                ht->items[index] = &HT_DELETED_ITEM;
            }
        }
        index = ht_get_hash(key, ht->size, i);
        item = ht->items[index];
        i++;
    }
    ht->count--;
    const int load = ht->count * 100 / ht->size;
    if (load < 10) {
        ht_resize_down(ht);
    }
}

void ht_del_hash_table() {
    _ht_del_hash_table(table);
}

void ht_insert(const char* key, texture_info value) {
    _ht_insert(table, key, value);
}

texture_info* ht_search(const char* key) {
    return _ht_search(table, key);
}

void ht_delete(const char* key) {
    _ht_delete(table, key);
}

void openPlist(const char* files[], int arrSize) {
    for (int i = 0; i < arrSize; i++) {
        FILE* file = fopen(files[i], "r");

        if (!file) {
            SYS_Report("Unable to open file\n");
            exit(0);
        }

        struct xml_document* document = xml_open_document(file);
        struct xml_node* plist = xml_document_root(document);
        struct xml_node* root = xml_node_child(plist, 0);
        struct xml_node* frames = xml_node_child(root, 1);

        parseFrames(frames, table, i);

        if (!document) {
            SYS_Report("Could not parse document\n");
            exit(0);
        }

        xml_document_free(document, true);
    }

    int bytes = table->size * (sizeof(ht_item) + sizeof(ht_item*)) + getStringSize();
    float percent = (float)bytes / (24 * 1024 * 1024);

    SYS_Report("Sprite sheet hash table size: %d items (about %d bytes, %.2f%% of MEM1)\n", table->size, bytes, 100 * percent);
}