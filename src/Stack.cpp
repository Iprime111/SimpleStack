#include <malloc.h>

#include "Stack.h"
#include "CustomAssert.h"
#include "ColorConsole.h"

StackErrorCode StackInit (Stack *stack, ssize_t initialCapacity) {
    PushLog (2);

    stack->capacity = initialCapacity;
    stack->size = 0;
    stack->data = (elem_t *) calloc ((size_t) stack->capacity, sizeof (elem_t));
    stack->errorCode = no_errors;

    VerifyStack (stack);

    RETURN no_errors;
}

StackErrorCode StackDestruct (Stack *stack) {
    PushLog (2);

    if (!stack){
        RETURN stack_pointer_null;
    }

    free (stack->data);

    stack->data = NULL;
    stack->capacity = -1;
    stack->size = -1;
    stack->errorCode = (StackErrorCode) (stack_pointer_null | data_pointer_null | invalid_capacity_value | anti_overflow | overflow | invalid_input);

    RETURN no_errors;
}

StackErrorCode StackRealloc (Stack *stack){
    PushLog (3);
    VerifyStack (stack);

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
            stack->errorCode = (StackErrorCode) (stack->errorCode | reallocation_error);
            RETURN stack->errorCode;
        }

        stack->data = testDataPointer;
    }

    VerifyStack (stack);

    RETURN stack->errorCode;
}

StackErrorCode StackPop (Stack *stack, elem_t *returnValue) {
    PushLog (2);

    custom_assert (returnValue != NULL, pointer_is_null, invalid_input);

    VerifyStack (stack);

    if (stack->size == 0){
        stack->errorCode = (StackErrorCode) (stack->errorCode | anti_overflow);

        DumpStack (stack);

        RETURN stack->errorCode;
    }

    *returnValue = stack->data [--stack->size];

    stack->errorCode = (StackErrorCode) (StackRealloc (stack) | stack->errorCode);

    RETURN stack->errorCode;
}

StackErrorCode StackPush (Stack *stack, elem_t value) {
    PushLog (2);
    VerifyStack (stack);

    stack->errorCode = (StackErrorCode) (stack->errorCode | StackRealloc (stack));

    if (stack->errorCode == no_errors)
        stack->data [stack->size++] = value;

    RETURN stack->errorCode;
}

StackErrorCode StackVerifier (Stack *stack) {
    PushLog (3);

    if (!stack){
        RETURN stack_pointer_null;
    }

    #define VerifyExpression_(errorExp, patternErrorCode) if (!(errorExp)) { (stack->errorCode) = (StackErrorCode) ((stack->errorCode) | patternErrorCode); }

    VerifyExpression_ (stack->data,                    data_pointer_null);
    VerifyExpression_ (stack->capacity >= 0,           invalid_capacity_value);
    VerifyExpression_ (stack->size >= 0,               anti_overflow);
    VerifyExpression_ (stack->size <= stack->capacity, overflow);


    #undef VerifyExpression_

    RETURN stack->errorCode;
}

void DumpErrors (const StackErrorCode errorCode, const char *function, const char *file, int line){
    custom_assert_without_logger (function != NULL, pointer_is_null,     (void)0);
    custom_assert_without_logger (file     != NULL, pointer_is_null,     (void)0);
    custom_assert_without_logger (function != file, not_enough_pointers, (void)0);

    fprintf_color (Console_red, Console_normal, stderr, "Stack has been corrupted in function %s (%s:%d)\n", function, file, line);

    fprintf_color (Console_red, Console_bold, stderr, "REPORT:\n");

    if (errorCode == no_errors){
        fprintf_color (Console_white, Console_bold, stderr, "No errors have been registered\n");
    }else{
        #define ERROR_MSG_(error, errorPattern, message) if (error & errorPattern){fprintf_color (Console_white, Console_bold, stderr, "%s: %s\n", #errorPattern, message);}

        ERROR_MSG_ (errorCode, stack_pointer_null,     "Pointer to stack variable has NULL value");
        ERROR_MSG_ (errorCode, data_pointer_null,      "Pointer to stack data has NULL value");
        ERROR_MSG_ (errorCode, invalid_capacity_value, "Stack has invalid capacity");
        ERROR_MSG_ (errorCode, overflow,               "Stack overflow has been occuried");
        ERROR_MSG_ (errorCode, anti_overflow,          "Stack anti-overflow has been occuried");
        ERROR_MSG_ (errorCode, allocation_error,       "Memory reallocation failed");

        #undef  ERROR_MSG_
    }

    fputs ("\n", stderr);
}

void DumpStackData (const Stack *stack){
    if (!stack){
        fprintf_color (Console_red, Console_bold, stderr, "Unable to read stack value\n");

        return;
    }

    fprintf_color (Console_red, Console_bold, stderr, "Stack (%p){\n", stack);

    #define PRINT_VARIABLE_(variable, printfSpecifier) do {\
                                                            fprintf_color (Console_green,  Console_bold, stderr, "\t%s (%p) = ", #variable, &(variable));\
                                                            fprintf_color (Console_purple, Console_bold, stderr, printfSpecifier "\n", variable);\
                                                        }while (0)

    ssize_t capacity = stack->capacity;
    ssize_t size = stack->size;

    PRINT_VARIABLE_ (size,     "%lld");
    PRINT_VARIABLE_ (capacity, "%lld");

    #undef PRINT_VARIABLE_

    fprintf_color (Console_purple, Console_normal, stderr, "\tdata (%p){\n ", stack->data);

    if (!stack->data){
        fprintf_color (Console_yellow, Console_bold, stderr, "\t\tUnable to read stack->data value\n}\n");
        fprintf_color (Console_red,    Console_bold, stderr, "}\n");

        return;
    }

    size_t realStackCapacity = malloc_usable_size (const_cast <elem_t *> (stack->data)) / sizeof (elem_t);

    for (size_t index = 0; index < realStackCapacity; index++){
        CONSOLE_COLOR dataColor = Console_red;

        if ((ssize_t) index < stack->size)
            dataColor = Console_green;

        fprintf_color (dataColor,     Console_normal, stderr, "\t\tdata [%lu] (%p) = ", index, stack->data + index);
        fprintf_color (Console_purple, Console_normal, stderr, ElementPrintfSpecifier "\n", stack->data [index]);

    }

    fprintf_color (Console_purple, Console_bold, stderr, "\t}\n");

    fprintf_color (Console_red,   Console_bold, stderr, "}\n");
}

void StackDump (const Stack *stack, const char *function, const char *file, int line) {
    custom_assert_without_logger (function != NULL, pointer_is_null,     (void)0);
    custom_assert_without_logger (file     != NULL, pointer_is_null,     (void)0);
    custom_assert_without_logger (function != file, not_enough_pointers, (void)0);

    DumpErrors (stack->errorCode, function, file, line);

    DumpStackData (stack);

    fprintf_color (Console_red, Console_bold, stderr, "\nBACKTRACE:\n");

    Show_stack_trace ();

}
