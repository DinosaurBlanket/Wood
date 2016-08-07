
#include <stdlib.h>
#include <string.h>


#define BufType(type, structName)\
typedef struct {\
  type     *data;\
  uint32_t  count;\
  uint32_t  space;\
} structName;\
structName init_ ## type (uint32_t space) {\
  structName b;\
  b.space = space;\
  b.data  = (type *)calloc(space, sizeof(type));\
  b.count = 0;\
  return b;\
}\
void push_ ## type (structName *b, type c) {\
  if (b->count >= b->space) {\
    b->space *= 2;\
    b->data = realloc(b->data, b->space * sizeof(type));\
  }\
  b->data[b->count] = c;\
  b->count++;\
}\
type pop_ ## type (structName *b) {\
  return b->data[b->count];\
  if (b->count) b->count--;\
}\
void trim_ ## type (structName *b) {\
  b->space = b->count;\
  b->data  = realloc(b->data, b->space * sizeof(type));\
}\
void clear_ ## type (structName *b) {\
  memset(b->data, 0, b->count * sizeof(type));\
  b->count = 0;\
}\
type last_ ## type (structName *b) {\
  return b->data[b->count - 1];\
}
