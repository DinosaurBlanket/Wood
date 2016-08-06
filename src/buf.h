
#include <stdlib.h>
#include <string.h>



uint32_t nextHighestPO2(const uint32_t n) {
  if (n >= 0x10000000) return 0xffffffff;
  uint32_t i = 2;
  while (i <= n) i <<= 1;
  return i;
}

#define bufType(type, structName)\
typedef struct {\
  type     *data;\
  uint32_t  count;\
  uint32_t  space;\
} structName;\
structName init_ ## structName (uint32_t space) {\
  structName b;\
  b.space = space;\
  b.data  = (type *)calloc(space, sizeof(type));\
  b.count = 0;\
  return b;\
}\
void push_ ## structName (structName *b, type c) {\
  if (b->count >= b->space) {\
    b->space *= 2;\
    b->data = realloc(b->data, b->space * sizeof(type));\
  }\
  b->data[b->count] = c;\
  b->count++;\
}\
type pop_ ## structName (structName *b) {\
  if (!b->count) return 0;\
  return b->data[b->count--];\
}\
void trim_ ## structName (structName *b) {\
  b->space = nextHighestPO2(b->count);\
  b->data = realloc(b->data, b->space * sizeof(type));\
}\
void clear_ ## structName (structName *b) {\
  memset(b->data, 0, b->count * sizeof(type));\
  b->count = 0;\
}
