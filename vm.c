#include "vm.h"
#include "chunk.h"
#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

VM vm;

static void resetStack() { vm.stackTop = vm.stack; }

static void runtimeError(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  size_t instruction = vm.ip - vm.chunk->code - 1;
  int line = vm.chunk->lines[instruction];
  fprintf(stderr, "[line %d] in script\n", line);
  resetStack();
}

void initVM() {
  resetStack();
  vm.objects = NULL;

  initTable(&vm.globals);
  initTable(&vm.strings);
}

void freeVM() {
  freeTable(&vm.globals);
  freeTable(&vm.strings);
  freeObjects();
}

void push(Value value) {
  *vm.stackTop = value;
  vm.stackTop++;
}

Value pop() {
  vm.stackTop--;
  return *vm.stackTop;
}

static Value peek(int distance) { return vm.stackTop[-1 - distance]; }

// Only nil and false are falsey in Lox
static bool isFalsey(Value value) {
  return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate() {
  ObjString *b = AS_STRING(pop());
  ObjString *a = AS_STRING(pop());

  // Calculate the length of the result string based on lengths of the operands
  int length = a->length + b->length;

  // Allocate a character array for the result, copy the two halves in, and
  // terminate the string
  char *chars = ALLOCATE(char, length + 1);
  memcpy(chars, a->chars, a->length);
  memcpy(chars + a->length, b->chars, b->length);
  chars[length] = '\0';

  // Produce an ObjString containing these characters
  ObjString *result = takeString(chars, length);
  push(OBJ_VAL(result));
}

static InterpretResult run() {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define READ_STRING() AS_STRING(READ_CONSTANT())
// this do/while pattern gives us a way to contain multiple statements inside
// a block, while also permitting semicolons at the end
#define BINARY_OP(valueType, op)                                               \
  do {                                                                         \
    if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {                          \
      runtimeError("Operands must be numbers.");                               \
      return INTERPRET_RUNTIME_ERROR;                                          \
    }                                                                          \
    double b = AS_NUMBER(pop());                                               \
    double a = AS_NUMBER(pop());                                               \
    push(valueType(a op b));                                                   \
  } while (false);
  for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
    printf("          ");
    for (Value *slot = vm.stack; slot < vm.stackTop; slot++) {
      printf("[ ");
      printValue(*slot);
      printf(" ]");
    }

    printf("\n");

    disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif

    uint8_t instruction;

    switch (instruction = READ_BYTE()) {
    case OP_CONSTANT: {
      Value constant = READ_CONSTANT();

      push(constant);

      break;
    }
    case OP_NIL: {
      push(NIL_VAL);
      break;
    }
    case OP_TRUE: {
      push(BOOL_VAL(true));
      break;
    }
    case OP_FALSE: {
      push(BOOL_VAL(false));
    }
    case OP_POP: {
      pop();
      break;
    }
    case OP_GET_GLOBAL: {
      ObjString *name = READ_STRING();
      Value value;

      if (!tableGet(&vm.globals, name, &value)) {
        runtimeError("Undefined variable '%s'.", name->chars);
        return INTERPRET_RUNTIME_ERROR;
      }

      push(value);
      break;
    }
    case OP_DEFINE_GLOBAL: {
      // Get variable name from constant table
      ObjString *name = READ_STRING();

      // Take value from top of stack and store in globals hash table
      tableSet(&vm.globals, name, peek(0));
      pop();
      break;
    }
    case OP_SET_GLOBAL: {
      ObjString *name = READ_STRING();

      if (tableSet(&vm.globals, name, peek(0))) {
        tableDelete(&vm.globals, name);
        runtimeError("Undefiend variable '%s'.", name->chars);
        return INTERPRET_RUNTIME_ERROR;
      }

      break;
    }
    case OP_EQUAL: {
      Value b = pop();
      Value a = pop();

      push(BOOL_VAL(valuesEqual(a, b)));
      break;
    }
    case OP_GREATER: {
      BINARY_OP(BOOL_VAL, >);
      break;
    }
    case OP_LESS: {
      BINARY_OP(BOOL_VAL, <);
      break;
    }
    case OP_ADD: {
      if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
        concatenate();
      } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
        double b = AS_NUMBER(pop());
        double a = AS_NUMBER(pop());

        push(NUMBER_VAL(a + b));
      } else {
        runtimeError("Operands must be two numbers or two strings.");
        return INTERPRET_RUNTIME_ERROR;
      }
      break;
    }
    case OP_SUBTRACT: {
      BINARY_OP(NUMBER_VAL, -);
      break;
    }
    case OP_MULTIPLY: {
      BINARY_OP(NUMBER_VAL, *);
      break;
    }
    case OP_DIVIDE: {
      BINARY_OP(NUMBER_VAL, /);
      break;
    }
    case OP_NOT: {
      push(BOOL_VAL(isFalsey(pop())));
      break;
    }
    case OP_NEGATE: {
      if (!IS_NUMBER(peek(0))) {
        runtimeError("Operand must be a number.");
        return INTERPRET_RUNTIME_ERROR;
      }

      push(NUMBER_VAL(-AS_NUMBER(pop())));
      break;
    }
    case OP_PRINT: {
      printValue(pop());
      printf("\n");
      break;
    }
    case OP_RETURN: {
      return INTERPRET_OK;
    }
    }
  }

#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
}

InterpretResult interpret(const char *source) {
  Chunk chunk;
  initChunk(&chunk);

  if (!compile(source, &chunk)) {
    freeChunk(&chunk);
    return INTERPRET_COMPILE_ERROR;
  }

  vm.chunk = &chunk;
  vm.ip = vm.chunk->code;

  InterpretResult result = run();

  freeChunk(&chunk);
  return result;
}
