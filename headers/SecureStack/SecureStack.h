#ifndef SECURE_STACK_H_
#define SECURE_STACK_H_

#include "Stack/Stack.h"
#include "Stack/StackHash.h"

const size_t PIPE_BUFFER_SIZE = 1024;
const size_t MAX_STACKS_COUNT = 1024;

#define StackInitDefaultSecure_(stack) StackInitSecure (stack, StackCallData{__PRETTY_FUNCTION__, __FILE__, __LINE__, #stack, true})
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
    OPERATION_PROCESSING        = 0,
    OPERATION_SUCCESS           = 1,
    OPERATION_FAILED            = -1,
    OPERATION_PROCESS_ERROR     = -2
};

enum MessageType {
    INFO_MESSAGE    = 0,
    ERROR_MESSAGE   = 1,
    WARNING_MESSAGE = 2,
    SUCCESS_MESSAGE = 3,
};

struct StackOperationRequest{
    StackCommand command;

    int stackDescriptor;

    elem_t argument;

    hash_t stackHash;
    hash_t dataHash;

    StackCommandResponse response;
};
#endif
