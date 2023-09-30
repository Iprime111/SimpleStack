#include <math.h>

#ifndef STACK_H_
#define STACK_H_

typedef double elem_t;

const ssize_t ReallocationScale = 2;
const ssize_t InitialCapacity = 10;

const size_t MaxAllocSize = 0x10000000000;

#ifdef _USE_CANARY
    typedef unsigned long long canary_t;

    #define leftCanaryPointer(stack)  ((canary_t *) (stack)->data - 1)
    #define rightCanaryPointer(stack) ((canary_t *) ((stack)->data + (stack)->capacity))

    const canary_t CanaryNormal = 0xFBADBEEF;
#endif

#ifndef _NDEBUG
    #define DumpStack(stack, stackCallData) StackDump (stack, __PRETTY_FUNCTION__, __FILE__, __LINE__, stackCallData)
#else
    #define DumpStack(stack, stackCallData) ;
#endif



#define VerifyStack(stack, stackCallData)                                                                                                                       \
    do {                                                                                                                                                        \
        StackErrorCode returnedError_ = StackVerifier (stack);                                                                                                  \
        if (returnedError_ != NO_ERRORS || ((stack) != NULL && (((stack)->errorCode = (StackErrorCode) (stack->errorCode | returnedError_)) != NO_ERRORS))) {   \
            DumpStack (stack, stackCallData);                                                                                                                   \
            RETURN (stack ? (stack)->errorCode : returnedError_);                                                                                               \
        }                                                                                                                                                       \
    }while (0)

enum StackErrorCode {
    NO_ERRORS              = 0,
    STACK_POINTER_NULL     = 1 << 0,
    DATA_POINTER_NULL      = 1 << 1,
    INVALID_CAPACITY_VALUE = 1 << 2,
    ANTI_OVERFLOW          = 1 << 3,
    OVERFLOW               = 1 << 4,
    INVALID_INPUT          = 1 << 5,
    REALLOCATION_ERROR     = 1 << 6,
    STACK_CANARY_CORRUPTED = 1 << 7,
    DATA_CANARY_CORRUPTED  = 1 << 8,
    WRONG_DATA_HASH        = 1 << 9,
    WRONG_STACK_HASH       = 1 << 10,
    SECURITY_PROCESS_ERROR = 1 << 11,
    EXTERNAL_VERIFY_FAILED = 1 << 12,
};


struct Stack {
    #ifdef _USE_CANARY
    canary_t leftCanary;
    #endif

    ssize_t capacity;
    ssize_t size;

    elem_t *data;

    StackErrorCode errorCode;

    #ifdef _USE_HASH
    unsigned long long stackHash;
    unsigned long long dataHash;
    #endif

    #ifdef _USE_CANARY
    canary_t rightCanary;
    #endif
};

struct StackCallData {
    const char *function;
    const char *file;

    const int line;

    const char *variableName;

    bool showDump;
};

#ifdef _USE_CANARY
    #define StackInitSize_(stack, initialCapacity) StackInit (stack, StackCallData{__PRETTY_FUNCTION__, __FILE__, __LINE__, #stack, true}, initialCapacity)
    #define StackInitDefault_(stack) StackInit (stack, StackCallData{__PRETTY_FUNCTION__, __FILE__, __LINE__, #stack, true})
#else
    #define StackInitSize_(stack, initialCapacity) StackInit (stack, StackCallData{__PRETTY_FUNCTION__, __FILE__, __LINE__, #stack, true}, initialCapacity)
    #define StackInitDefault_(stack) StackInit (stack, StackCallData{__PRETTY_FUNCTION__, __FILE__, __LINE__, #stack, true})
#endif



#define StackDestruct_(stack) StackDestruct (stack)

#define StackPop_(stack, returnValue) StackPop (stack, returnValue, StackCallData{__PRETTY_FUNCTION__, __FILE__, __LINE__, #stack, true})
#define StackPush_(stack, value)     StackPush (stack, value,       StackCallData{__PRETTY_FUNCTION__, __FILE__, __LINE__, #stack, true})


StackErrorCode StackInit (Stack *stack, const StackCallData callData, ssize_t initialCapacity = InitialCapacity);

StackErrorCode StackDestruct (Stack *stack);


StackErrorCode StackPop (Stack *stack, elem_t *returnValue, const StackCallData callData);

StackErrorCode StackPush (Stack *stack, elem_t value, const StackCallData callData);
#endif

