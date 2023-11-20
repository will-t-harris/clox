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

static int constantInstruction(const char *name, Chunk *chunk, int offset) {
  uint8_t constant = chunk->code[offset + 1];

  printf("%-16s %4d '", name, constant);
  printValue(chunk->constants.values[constant]);
  printf("'\n");

  // return the offset of the next instruction
  return offset + 2;
}

static int simpleInstruction(const char *name, int offset) {
  printf("%s\n", name);

  // return the offset of the next instruction
  return offset + 1;
}

int disassembleInstruction(Chunk *chunk, int offset) {
  printf("%04d ", offset);
  if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1]) {
    // show a pipe character for any instruction that comes from the same source
    // line as the preceding one, since sometimes a single line of source code
    // can result in many bytecode instructions
    printf("   | ");
  } else {
    printf("%4d ", chunk->lines[offset]);
  }

  uint8_t instruction = chunk->code[offset];
  switch (instruction) {
  case OP_CONSTANT: {
    return constantInstruction("OP_CONSTANT", chunk, offset);
  }
  case OP_RETURN: {
    return simpleInstruction("OP_RETURN", offset);
  }
  default: {
    printf("unknown opcode %d\n", instruction);
    return offset + 1;
  }
  }
}
