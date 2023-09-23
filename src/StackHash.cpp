#include "StackHash.h"
#include "Logger.h"
#include <cstdio>

unsigned long long HashFunction (char *array, size_t length);

StackErrorCode CompareStackHash (Stack *stack){
    PushLog (3);

    if (!stack){
        RETURN STACK_POINTER_NULL;
    }

    hash_t newHash = 0;
    hash_t oldHash = stack->stackHash;

    StackErrorCode errorCode = ComputeStackHash(stack, &newHash);

    if (errorCode != NO_ERRORS) {
        RETURN errorCode;
    }

    stack->stackHash = oldHash;

    if (newHash != oldHash){
        RETURN WRONG_STACK_HASH;
    }

    RETURN NO_ERRORS;
}

StackErrorCode CompareDataHash (Stack *stack, size_t dataLength) {
    PushLog (3);

    if (!stack){
        RETURN STACK_POINTER_NULL;
    }

    hash_t newHash = 0;

    StackErrorCode errorCode = ComputeDataHash(stack, dataLength, &newHash);

    if (errorCode != NO_ERRORS) {
        RETURN errorCode;
    }

    if (newHash != stack->dataHash){
        RETURN WRONG_DATA_HASH;
    }

    RETURN NO_ERRORS;
}

StackErrorCode ComputeStackHash (Stack *stack, hash_t *hash) {
    PushLog (3);

    if (!stack){
        RETURN STACK_POINTER_NULL;
    }

    hash_t stackHash = stack->stackHash;
    hash_t  dataHash = stack->dataHash;

    stack->dataHash  = 0;
    stack->stackHash = 0;

    *hash = HashFunction((char *) stack, sizeof (Stack));

    if (hash != &(stack->stackHash))
        stack->stackHash = stackHash;

    stack->dataHash  = dataHash;

    RETURN NO_ERRORS;
}

StackErrorCode ComputeDataHash (Stack *stack, size_t dataLength, hash_t *hash) {
    PushLog (3);

    if (!stack){
        RETURN STACK_POINTER_NULL;
    }

    if (!stack->data){
        RETURN DATA_POINTER_NULL;
    }

    #ifdef _USE_CANARY
        canary_t *dataPointer = (canary_t *) stack->data - 1;
    #else
        elem_t   *dataPointer = stack->data;
    #endif

    *hash = HashFunction((char *) dataPointer, dataLength);

    RETURN NO_ERRORS;
}

hash_t HashFunction (char *array, size_t length) {
    PushLog (4);

    hash_t hash = 5381;

    for (size_t index = 0; index < length; index++){
        hash = ((hash << 5) + hash) + (hash_t) array [index];
    }

    RETURN hash;
}
