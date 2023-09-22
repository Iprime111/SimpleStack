#include <math.h>

#ifndef STACK_H_
#define STACK_H_

typedef double elem_t;

const elem_t PoisonValue = NAN;
const ssize_t ReallocationScale = 2;
const ssize_t InitialCapacity = 10;

#define ElementPrintfSpecifier "%lf"


#ifndef _NDEBUG
    #define DumpStack(stack, stackCallData) StackDump (stack, __PRETTY_FUNCTION__, __FILE__, __LINE__, stackCallData)
#else
    #define DumpStack(stack, stackCallData) ;
#endif

#define VerifyStack(stack, stackCallData)                                                                               \
                            do {                                                                                        \
                                (stack)->errorCode = (StackErrorCode) (StackVerifier (stack) | (stack)->errorCode);     \
                                if ((stack)->errorCode != no_errors) {                                                  \
                                    DumpStack (stack, stackCallData);                                                   \
                                    RETURN (stack)->errorCode;                                                          \
                                }                                                                                       \
                            }while (0)

enum StackErrorCode {
    no_errors              = 0,
    stack_pointer_null     = 1 << 0,
    data_pointer_null      = 1 << 1,
    invalid_capacity_value = 1 << 2,
    anti_overflow          = 1 << 3,
    overflow               = 1 << 4,
    invalid_input          = 1 << 5,
    reallocation_error     = 1 << 6,
};

struct Stack {
    ssize_t capacity;
    ssize_t size;

    elem_t *data;

    StackErrorCode errorCode;
};

struct StackCallData {
    const char *function;
    const char *file;

    const int line;

    const char *variableName;
};

#define StackInitSize_(stack, initialCapacity) StackInit (stack, StackCallData{__PRETTY_FUNCTION__, __FILE__, __LINE__, #stack}, initialCapacity)
#define StackInitDefault_(stack) StackInit (stack, StackCallData{__PRETTY_FUNCTION__, __FILE__, __LINE__, #stack})

#define StackDestruct_(stack) StackDestruct (stack)

#define StackPop_(stack, returnValue) StackPop (stack, returnValue, StackCallData{__PRETTY_FUNCTION__, __FILE__, __LINE__, #stack})
#define StackPush_(stack, value)     StackPush (stack, value,       StackCallData{__PRETTY_FUNCTION__, __FILE__, __LINE__, #stack})

StackErrorCode StackInit (Stack *stack, const StackCallData callData, ssize_t initialCapacity = InitialCapacity);
StackErrorCode StackDestruct (Stack *stack);

StackErrorCode StackPop (Stack *stack, elem_t *returnValue, const StackCallData callData);
StackErrorCode StackPush (Stack *stack, elem_t value, const StackCallData callData);

#endif

