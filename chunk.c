#include <stdint.h>
#include <stdlib.h>

#include "chunk.h"
#include "memory.h"

// create empty dynamic array
void initChunk(Chunk *chunk) {
  chunk->count = 0;
  chunk->capacity = 0;
  chunk->code = NULL;
}

void freeChunk(Chunk *chunk) {
  FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);

  initChunk(chunk);
}

// append to dynamic array
void writeChunk(Chunk *chunk, uint8_t byte) {
  if (chunk->capacity < chunk->count + 1) {
    // grow array if already full
    int oldCapacity = chunk->capacity;

    chunk->capacity = GROW_CAPACITY(oldCapacity);

    chunk->code =
        GROW_ARRAY(uint8_t, chunk->code, oldCapacity, chunk->capacity);
  }

  chunk->code[chunk->count] = byte;
  chunk->count++;
}
