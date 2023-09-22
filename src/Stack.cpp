#include <malloc.h>

#include "Stack.h"
#include "CustomAssert.h"
#include "ColorConsole.h"

StackErrorCode StackRealloc (Stack *stack, const StackCallData callData);

StackErrorCode StackVerifier (Stack *stack);
void StackDump (const Stack *stack, const char *function, const char *file, int line, const StackCallData callData);
void DumpErrors (const StackErrorCode errorCode, const char *function, const char *file, int line, const StackCallData callData);
void DumpStackData (const Stack *stack);

StackErrorCode StackInit (Stack *stack, const StackCallData callData, ssize_t initialCapacity) {
    PushLog (2);

    stack->capacity = initialCapacity;
    stack->size = 0;
    stack->data = (elem_t *) calloc ((size_t) stack->capacity, sizeof (elem_t));
    stack->errorCode = NO_ERRORS;

    VerifyStack (stack, callData);

    RETURN NO_ERRORS;
}

StackErrorCode StackDestruct (Stack *stack) {
    PushLog (2);

    if (!stack){
        RETURN STACK_POINTER_NULL;
    }

    if (stack->data){
        size_t realStackCapacity = malloc_usable_size (const_cast <elem_t *> (stack->data)) / sizeof (elem_t);

        for (size_t index = 0; index < realStackCapacity; index++){
            stack->data [index] = PoisonValue;
        }

        free (stack->data);
    }

    stack->data = NULL;
    stack->capacity = -1;
    stack->size = -1;
    stack->errorCode = (StackErrorCode) (STACK_POINTER_NULL | DATA_POINTER_NULL | INVALID_CAPACITY_VALUE | ANTI_OVERFLOW | OVERFLOW | INVALID_INPUT | REALLOCATION_ERROR);

    RETURN NO_ERRORS;
}

StackErrorCode StackRealloc (Stack *stack, const StackCallData callData){
    PushLog (3);
    VerifyStack (stack, callData);

    bool shouldRealloc = false;

    if (stack->size == stack->capacity){
        stack->capacity *= ReallocationScale;
        shouldRealloc = true;
    }

    if (stack->size < stack->capacity / (ReallocationScale * ReallocationScale)){
        stack->capacity /= ReallocationScale;
        shouldRealloc = true;
    }

    if (shouldRealloc){
        elem_t *testDataPointer = (elem_t *) realloc (stack->data, (size_t) stack->capacity * sizeof (elem_t));
        if (!testDataPointer){
            stack->errorCode = (StackErrorCode) (stack->errorCode | REALLOCATION_ERROR);
            RETURN stack->errorCode;
        }

        stack->data = testDataPointer;
    }

    VerifyStack (stack, callData);

    RETURN stack->errorCode;
}

StackErrorCode StackPop (Stack *stack, elem_t *returnValue, const StackCallData callData) {
    PushLog (2);

    custom_assert (returnValue != NULL, pointer_is_null, INVALID_INPUT);

    VerifyStack (stack, callData);

    if (stack->size == 0){
        stack->errorCode = (StackErrorCode) (stack->errorCode | ANTI_OVERFLOW);

        DumpStack (stack, callData);

        RETURN stack->errorCode;
    }

    *returnValue = stack->data [--stack->size];

    stack->errorCode = (StackErrorCode) (StackRealloc (stack, callData) | stack->errorCode);

    RETURN stack->errorCode;
}

StackErrorCode StackPush (Stack *stack, elem_t value, const StackCallData callData) {
    PushLog (2);
    VerifyStack (stack, callData);

    stack->errorCode = (StackErrorCode) (stack->errorCode | StackRealloc (stack, callData));

    if (stack->errorCode == NO_ERRORS)
        stack->data [stack->size++] = value;

    RETURN stack->errorCode;
}

StackErrorCode StackVerifier (Stack *stack) {
    PushLog (3);

    if (!stack){
        RETURN STACK_POINTER_NULL;
    }

    #define VerifyExpression_(errorExp, patternErrorCode)                                                                   \
                                        if (!(errorExp)) {                                                                  \
                                            (stack->errorCode) = (StackErrorCode) ((stack->errorCode) | patternErrorCode);  \
                                        }

    VerifyExpression_ (stack->data,                    DATA_POINTER_NULL);
    VerifyExpression_ (stack->capacity >= 0,           INVALID_CAPACITY_VALUE);
    VerifyExpression_ (stack->size >= 0,               ANTI_OVERFLOW);
    VerifyExpression_ (stack->size <= stack->capacity, OVERFLOW);


    #undef VerifyExpression_

    RETURN stack->errorCode;
}

void DumpErrors (const StackErrorCode errorCode, const char *function, const char *file, int line, const StackCallData callData){
    custom_assert_without_logger (function != NULL, pointer_is_null,     (void)0);
    custom_assert_without_logger (file     != NULL, pointer_is_null,     (void)0);
    custom_assert_without_logger (function != file, not_enough_pointers, (void)0);

    fprintf_color (CONSOLE_RED, CONSOLE_NORMAL, stderr, "Stack has been corrupted in function %s (%s:%d)\n", function, file, line);
    fprintf_color (CONSOLE_RED, CONSOLE_NORMAL, stderr, "Stack variable name: \"%s\".Called from %s (%s:%d)\n", callData.variableName, callData.function, callData.file, callData.line);

    fprintf_color (CONSOLE_RED, CONSOLE_BOLD, stderr, "REPORT:\n");

    if (errorCode == NO_ERRORS){
        fprintf_color (CONSOLE_WHITE, CONSOLE_BOLD, stderr, "No errors have been registered\n");
    }else{
        #define ERROR_MSG_(error, errorPattern, message)                                                                                \
                                            if (error & errorPattern){                                                                  \
                                                fprintf_color (CONSOLE_WHITE, CONSOLE_BOLD, stderr, "%s: %s\n", #errorPattern, message);\
                                            }

        ERROR_MSG_ (errorCode, STACK_POINTER_NULL,     "Pointer to stack variable has NULL value");
        ERROR_MSG_ (errorCode, DATA_POINTER_NULL,      "Pointer to stack data has NULL value");
        ERROR_MSG_ (errorCode, INVALID_CAPACITY_VALUE, "Stack has invalid capacity");
        ERROR_MSG_ (errorCode, OVERFLOW,               "Stack OVERFLOW has been occuried");
        ERROR_MSG_ (errorCode, ANTI_OVERFLOW,          "Stack anti-OVERFLOW has been occuried");
        ERROR_MSG_ (errorCode, allocation_error,       "Memory reallocation failed");

        #undef  ERROR_MSG_
    }

    fputs ("\n", stderr);
}

void DumpStackData (const Stack *stack){
    if (!stack){
        fprintf_color (CONSOLE_RED, CONSOLE_BOLD, stderr, "Unable to read stack value\n");

        return;
    }

    fprintf_color (CONSOLE_RED, CONSOLE_BOLD, stderr, "Stack (%p){\n", stack);

    #define PRINT_VARIABLE_(variable, printfSpecifier)                                                                                              \
                                                do {                                                                                                \
                                                    fprintf_color (CONSOLE_GREEN,  CONSOLE_BOLD, stderr, "\t%s (%p) = ", #variable, &(variable));   \
                                                    fprintf_color (CONSOLE_PURPLE, CONSOLE_BOLD, stderr, printfSpecifier "\n", variable);           \
                                                }while (0)

    ssize_t capacity = stack->capacity;
    ssize_t size = stack->size;

    PRINT_VARIABLE_ (size,     "%lld");
    PRINT_VARIABLE_ (capacity, "%lld");

    #undef PRINT_VARIABLE_

    fprintf_color (CONSOLE_PURPLE, CONSOLE_NORMAL, stderr, "\tdata (%p){\n ", stack->data);

    if (!stack->data){
        fprintf_color (CONSOLE_YELLOW, CONSOLE_BOLD, stderr, "\t\tUnable to read stack->data value\n}\n");
        fprintf_color (CONSOLE_RED,    CONSOLE_BOLD, stderr, "}\n");

        return;
    }

    size_t realStackCapacity = malloc_usable_size (const_cast <elem_t *> (stack->data)) / sizeof (elem_t);

    for (size_t index = 0; index < realStackCapacity; index++){
        CONSOLE_COLOR dataColor = CONSOLE_RED;

        if ((ssize_t) index < stack->size)
            dataColor = CONSOLE_GREEN;

        fprintf_color (dataColor,     CONSOLE_NORMAL, stderr, "\t\tdata [%lu] (%p) = ", index, stack->data + index);
        fprintf_color (CONSOLE_PURPLE, CONSOLE_NORMAL, stderr, ElementPrintfSpecifier "\n", stack->data [index]);

    }

    fprintf_color (CONSOLE_PURPLE, CONSOLE_BOLD, stderr, "\t}\n");

    fprintf_color (CONSOLE_RED,   CONSOLE_BOLD, stderr, "}\n");
}

void StackDump (const Stack *stack, const char *function, const char *file, int line, const StackCallData callData) {
    custom_assert_without_logger (function != NULL, pointer_is_null,     (void)0);
    custom_assert_without_logger (file     != NULL, pointer_is_null,     (void)0);
    custom_assert_without_logger (function != file, not_enough_pointers, (void)0);

    DumpErrors (stack->errorCode, function, file, line, callData);

    DumpStackData (stack);

    fprintf_color (CONSOLE_RED, CONSOLE_BOLD, stderr, "\nBACKTRACE:\n");

    Show_stack_trace ();

}
