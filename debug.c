#include <stdio.h>

#include "chunk.h"
#include "debug.h"

void disassembleChunk(Chunk *chunk, const char *name) {
  printf("== %s ==\n", name);
  for (int offset = 0; offset < chunk->count;) {
    // disassembleInstruction handles incrememting this loop's counter because
    // instructions can have different sizes
    offset = disassembleInstruction(chunk, offset);
  }
}

static int simpleInstruction(const char *name, int offset) {
  printf("%s\n", name);
  return offset + 1;
}

int disassembleInstruction(Chunk *chunk, int offset) {
  printf("%04d ", offset);

  uint8_t instruction = chunk->code[offset];
  switch (instruction) {
  case OP_RETURN: {
    return simpleInstruction("OP_RETURN", offset);
  }
  default: {
    printf("unknown opcode %d\n", instruction);
    return offset + 1;
  }
  }
}
