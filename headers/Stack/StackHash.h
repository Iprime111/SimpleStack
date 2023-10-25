#include "Stack/Stack.h"

#ifndef STACK_HASH_H_
#define STACK_HASH_H_

typedef unsigned long long hash_t;

StackErrorCode CompareStackHash (Stack *stk);

StackErrorCode CompareDataHash (Stack *stk, size_t dataLength);

StackErrorCode ComputeStackHash (Stack *stack, hash_t *hash);

StackErrorCode ComputeDataHash (Stack *stack, size_t dataLength, hash_t *hash);

#endif
