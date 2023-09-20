#include <math.h>

#ifndef STACK_H_
#define STACK_H_

typedef double elem_t;

const elem_t PoisonValue = NAN;
const ssize_t ReallocationScale = 2;
const ssize_t InitialCapacity = 10;

#define ElementPrintfSpecifier "%lf"


#ifndef _NDEBUG
    #define DumpStack(stack, error_code) StackDump (stack, error_code, __PRETTY_FUNCTION__, __FILE__, __LINE__)
#else
    #define DumpStack(stack, error_code) ;
#endif

#define VerifyStack(stack)  do {                                                              \
                                Stack_error_code stack_verify_result_ = StackVerifier (stack);\
                                if (stack_verify_result_ != no_errors) {                      \
                                    DumpStack (stack, stack_verify_result_);                  \
                                    RETURN stack_verify_result_;                              \
                                }                                                             \
                            }while (0)

enum Stack_error_code {
    no_errors              = 0,
    stack_pointer_null     = 1 << 0,
    data_pointer_null      = 1 << 1,
    invalid_capacity_value = 1 << 2,
    anti_overflow          = 1 << 3,
    overflow               = 1 << 4,
    invalid_input          = 1 << 5,
};

struct Stack {
    ssize_t capacity;
    ssize_t size;

    elem_t *data;
};

Stack_error_code StackInit (Stack *stack, ssize_t initial_capacity = InitialCapacity);
Stack_error_code StackDestruct (Stack *stack);
Stack_error_code StackRealloc (Stack *stack);

Stack_error_code StackPop (Stack *stack, elem_t *return_value);
Stack_error_code StackPush (Stack *stack, elem_t value);

Stack_error_code StackVerifier (const Stack *stack);
void StackDump (const Stack *stack, const Stack_error_code error_code, const char *function, const char *file, int line);
void DumpErrors (const Stack_error_code error_code, const char *function, const char *file, int line);
void DumpStackData (const Stack *stack);

#endif

