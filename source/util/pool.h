#pragma once

#include <stddef.h>

typedef struct pool_slot {
    struct pool_slot* next;
    struct pool_slot* prev;
} PoolSlot;

typedef struct {
    PoolSlot* slots;
    size_t size;
    PoolSlot* free_list;
} Pool;

void Pool_Init(Pool* pool, PoolSlot* slots, size_t num_slots);

int Pool_Acquire(Pool* pool, PoolSlot** active_list_ptr);
void Pool_Release(Pool* pool, unsigned int index, PoolSlot** active_list_ptr);
