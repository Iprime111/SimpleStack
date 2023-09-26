#ifndef SECURE_STACK_H_
#define SECURE_STACK_H_

#include "Stack.h"

const size_t PIPE_BUFFER_SIZE = 1024;
const size_t MAX_STACKS_COUNT = 1024;

#define StackInitDefaultSecure_(stack) StackInitSecure (stack, StackCallData{__PRETTY_FUNCTION__, __FILE__, __LINE__, #stack, true})
#define StackInitSizeSecure_(stack, initialCapacity) StackInitSecure (stack, StackCallData{__PRETTY_FUNCTION__, __FILE__, __LINE__, #stack, true}, initialCapacity)
#define StackDestructSecure_(stack) StackDestructSecure (stack)

#define StackPopSecure_(stack, returnValue) StackPopSecure (stack, returnValue, StackCallData{__PRETTY_FUNCTION__, __FILE__, __LINE__, #stack, true})
#define StackPushSecure_(stack, value) StackPushSecure (stack, value, StackCallData{__PRETTY_FUNCTION__, __FILE__, __LINE__, #stack, true})

StackErrorCode SecurityProcessInit ();
StackErrorCode SecurityProcessStop ();

StackErrorCode StackInitSecure (Stack *stack, const StackCallData callData, ssize_t initialCapacity=InitialCapacity);
StackErrorCode StackDestructSecure (Stack *stack);

StackErrorCode StackPopSecure (Stack *stack, elem_t *returnValue, const StackCallData callData);
StackErrorCode StackPushSecure (Stack *stack, elem_t value, const StackCallData callData);

enum StackCommand {
    UNKNOWN_COMMAND          = 0,
    STACK_POP_COMMAND        = 1,
    STACK_PUSH_COMMAND       = 2,
    STACK_INIT_COMMAND       = 3,
    STACK_DESTRUCT_COMMAND   = 4,
    STACK_VERIFY_COMMAND     = 5,
    ABORT_PROCESS            = 6,
};

enum StackCommandResponse {
    OPERATION_SUCCESS   = 0,
    OPERATION_FAILED    = 1,
};

#endif
