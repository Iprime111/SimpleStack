#include <math.h>

#ifndef STACK_H_
#define STACK_H_

typedef double elem_t;

const elem_t PoisonValue = NAN;
const ssize_t ReallocationScale = 2;
const ssize_t InitialCapacity = 10;

#define ElementPrintfSpecifier "%lf"


#ifndef _NDEBUG
    #define DumpStack(stack) StackDump (stack, __PRETTY_FUNCTION__, __FILE__, __LINE__)
#else
    #define DumpStack(stack) ;
#endif

#define VerifyStack(stack)  do {                                                                                        \
                                (stack)->errorCode = (StackErrorCode) (StackVerifier (stack) | (stack)->errorCode);     \
                                if ((stack)->errorCode != no_errors) {                                                  \
                                    DumpStack (stack);                                                                  \
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

StackErrorCode StackInit (Stack *stack, ssize_t initialCapacity = InitialCapacity);
StackErrorCode StackDestruct (Stack *stack);
StackErrorCode StackRealloc (Stack *stack);

StackErrorCode StackPop (Stack *stack, elem_t *return_value);
StackErrorCode StackPush (Stack *stack, elem_t value);

StackErrorCode StackVerifier (Stack *stack);
void StackDump (const Stack *stack, const char *function, const char *file, int line);
void DumpErrors (const StackErrorCode errorCode, const char *function, const char *file, int line);
void DumpStackData (const Stack *stack);

#endif

