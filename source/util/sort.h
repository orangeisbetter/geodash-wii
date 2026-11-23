#pragma once

#include <stddef.h>

void merge_sort(void* arr, size_t n, size_t elem_size, int (*cmp)(const void*, const void*));