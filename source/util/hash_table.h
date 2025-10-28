#pragma once

#include <stdbool.h>

typedef struct {
    float x;
    float y;
} vec2;

typedef struct {
    int sheetIdx;
    vec2 spriteOffset;
    vec2 spriteSize;
    vec2 spriteSourceSize;
    struct {
        vec2 pos;
        vec2 size;
    } textureRect;
    bool textureRotated;
} texture_info;

typedef struct {
    char* key;
    texture_info value;
} ht_item;

typedef struct {
    int size;
    int count;
    int base_size;
    ht_item** items;
} ht_hash_table;

void ht_init();

void ht_del_hash_table();
void ht_insert(const char* key, texture_info value);
texture_info* ht_search(const char* key);
void ht_delete(const char* key);

void openPlist(const char* files[], int arrSize);