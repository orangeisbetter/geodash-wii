#include "pool.h"

#include <malloc.h>

void Pool_Init(Pool* pool, PoolSlot* slots, size_t num_slots) {
    pool->slots = slots;
    pool->size = num_slots;

    for (size_t i = 0; i < pool->size; i++) {
        pool->slots[i].prev = i == 0              ? NULL : &pool->slots[i - 1];
        pool->slots[i].next = i == pool->size - 1 ? NULL : &pool->slots[i + 1];
    }

    pool->free_list = &pool->slots[0];
}

int Pool_Acquire(Pool* pool, PoolSlot** active_list_ptr) {
    PoolSlot* slot = pool->free_list;
    if (!slot) {
        return -1;
    }

    pool->free_list = slot->next;
    // no need to update prev of next node, free list is never traversed backwards

    slot->next = *active_list_ptr;
    slot->prev = NULL;
    if (*active_list_ptr) {
        (*active_list_ptr)->prev = slot;
    }
    *active_list_ptr = slot;

    return (int) (slot - pool->slots);
}

void Pool_Release(Pool* pool, unsigned int index, PoolSlot** active_list_ptr) {
    PoolSlot* slot = pool->slots + index;

    // link incoming next
    if (slot->prev) {
        slot->prev->next = slot->next;
    } else {
        *active_list_ptr = slot->next;
    }

    // link incoming prev
    if (slot->next) {
        slot->next->prev = slot->prev;
    }

    // Make head of free list
    slot->next = pool->free_list;
    pool->free_list = slot;
}
