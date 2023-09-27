#include "Stack/Stack.h"

#ifndef STACK_HASH_H_
#define STACK_HASH_H_

typedef unsigned long long hash_t;

template <typename elem_t>
StackErrorCode CompareStackHash (Stack <elem_t> *stk);
template <typename elem_t>
StackErrorCode CompareDataHash (Stack <elem_t> *stk, size_t dataLength);

template <typename elem_t>
StackErrorCode ComputeStackHash (Stack <elem_t> *stack, hash_t *hash);
template <typename elem_t>
StackErrorCode ComputeDataHash (Stack <elem_t> *stack, size_t dataLength, hash_t *hash);

#endif
