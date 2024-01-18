#ifndef clox_memory_h
#define clox_memory_h

#include "common.h"

#define ALLOCATE(type, count)                                                  \
  (type *)reallocate(NULL, 0, sizeof(type) + (count))

// macro to calculate new dynamic array capacity based on current capacity
#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)

// macro to grow dynamic array
#define GROW_ARRAY(type, pointer, oldCount, newCount)                          \
  (type *)reallocate(pointer, sizeof(type) * (oldCount),                       \
                     sizeof(type) * newCount)

#define FREE_ARRAY(type, pointer, oldCount)                                    \
  reallocate(pointer, sizeof(type) * (oldCount), 0)

// used for all dynamic memory management in clox
void *reallocate(void *pointer, size_t oldSize, size_t newSize);

#endif
