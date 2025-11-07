#include "sort.h"

#include <stdlib.h>
#include <string.h>

// Merge two halves: left [l..m], right [m+1..r]
static void merge(void* arr, void* temp, size_t l, size_t m, size_t r, size_t elem_size, int (*cmp)(const void*, const void*)) {
    size_t i = l, j = m + 1, k = l;

    while (i <= m && j <= r) {
        if (cmp((char*)arr + i * elem_size, (char*)arr + j * elem_size) <= 0) {
            memcpy((char*)temp + k * elem_size, (char*)arr + i * elem_size, elem_size);
            i++;
        } else {
            memcpy((char*)temp + k * elem_size, (char*)arr + j * elem_size, elem_size);
            j++;
        }
        k++;
    }

    // Copy remaining elements from left half
    while (i <= m) {
        memcpy((char*)temp + k * elem_size, (char*)arr + i * elem_size, elem_size);
        i++;
        k++;
    }

    // Copy remaining elements from right half
    while (j <= r) {
        memcpy((char*)temp + k * elem_size, (char*)arr + j * elem_size, elem_size);
        j++;
        k++;
    }

    // Copy merged result back to original array
    for (i = l; i <= r; i++) {
        memcpy((char*)arr + i * elem_size, (char*)temp + i * elem_size, elem_size);
    }
}

static void merge_sort_recursive(void* arr, void* temp, size_t l, size_t r, size_t elem_size, int (*cmp)(const void*, const void*)) {
    if (l >= r)
        return;

    size_t m = l + (r - l) / 2;
    merge_sort_recursive(arr, temp, l, m, elem_size, cmp);
    merge_sort_recursive(arr, temp, m + 1, r, elem_size, cmp);
    merge(arr, temp, l, m, r, elem_size, cmp);
}

void merge_sort(void* arr, size_t n, size_t elem_size, int (*cmp)(const void*, const void*)) {
    if (n <= 1)
        return;

    void* temp = malloc(n * elem_size);
    if (!temp)
        return;  // malloc failure, silently fail

    merge_sort_recursive(arr, temp, 0, n - 1, elem_size, cmp);

    free(temp);
}
