#include <malloc.h>

#include "Stack.h"
#include "CustomAssert.h"
#include "ColorConsole.h"

Stack_error_code StackInit (Stack *stack) {
    PushLog (2);

    stack->capacity = InitialCapacity;
    stack->size = 0;
    stack->data = (elem_t *) calloc ((size_t) stack->capacity, sizeof (elem_t));

    VerifyStack (stack);

    RETURN no_errors;
}

Stack_error_code StackDestruct (Stack *stack) {
    PushLog (2);

    if (!stack){
        RETURN stack_pointer_null;
    }

    free (stack->data);

    stack->data = NULL;
    stack->capacity = -1;
    stack->size = -1;

    RETURN no_errors;
}

Stack_error_code StackRealloc (Stack *stack){
    PushLog (3);
    VerifyStack (stack);

    if (stack->size == stack->capacity){
        stack->capacity *= ReallocationScale;
        stack->data = (elem_t *) realloc (stack->data, (size_t) stack->capacity * sizeof (elem_t));
    }

    if (stack->size < stack->capacity / ReallocationScale){
        stack->capacity /= ReallocationScale;
        stack->data = (elem_t *) realloc (stack->data, (size_t) stack->capacity * sizeof (elem_t));
    }

    VerifyStack (stack);

    RETURN no_errors;
}

Stack_error_code StackPop (Stack *stack, elem_t *return_value) {
    PushLog (2);

    custom_assert (return_value != NULL, pointer_is_null, invalid_input);

    VerifyStack (stack);

    Stack_error_code error_code = no_errors;

    if (stack->size == 0){
        DumpStack (stack, anti_overflow);

        RETURN anti_overflow;
    }

    *return_value = stack->data [--stack->size];

    error_code = (Stack_error_code) (StackRealloc (stack) | error_code);

    RETURN error_code;
}

Stack_error_code StackPush (Stack *stack, elem_t value) {
    PushLog (2);
    VerifyStack (stack);

    Stack_error_code error_code = no_errors;

    error_code = (Stack_error_code) (StackRealloc (stack) | error_code);

    stack->data [stack->size++] = value;

    RETURN error_code;
}

Stack_error_code StackVerifier (const Stack *stack) {
    PushLog (3);

    Stack_error_code RETURN_code = no_errors;

    if (!stack){
        RETURN stack_pointer_null;
    }

    #define VerifyExpression_(error_exp, error_code) if (!(error_exp)) { RETURN_code = (Stack_error_code) (RETURN_code | error_code); }

    VerifyExpression_ (stack->data,                    data_pointer_null);
    VerifyExpression_ (stack->capacity >= 0,           invalid_capacity_value);
    VerifyExpression_ (stack->size >= 0,               anti_overflow);
    VerifyExpression_ (stack->size <= stack->capacity, overflow);


    #undef VerifyExpression_

    RETURN RETURN_code;
}

void DumpErrors (const Stack_error_code error_code, const char *function, const char *file, int line){
    custom_assert_without_logger (function != NULL, pointer_is_null,     (void)0);
    custom_assert_without_logger (file     != NULL, pointer_is_null,     (void)0);
    custom_assert_without_logger (function != file, not_enough_pointers, (void)0);

    fprintf_color (Console_red, Console_normal, stderr, "Stack has been corrupted in function %s (%s:%d)\n", function, file, line);

    fprintf_color (Console_red, Console_bold, stderr, "REPORT:\n");

    if (error_code == no_errors){
        fprintf_color (Console_white, Console_bold, stderr, "No errors have been registered\n");
    }else{
        #define ERROR_MSG_(error, error_pattern, message) if (error & error_pattern){fprintf_color (Console_white, Console_bold, stderr, "%s: %s\n", #error_pattern, message);}

        ERROR_MSG_ (error_code, stack_pointer_null,     "Pointer to stack variable has NULL value");
        ERROR_MSG_ (error_code, data_pointer_null,      "Pointer to stack data has NULL value");
        ERROR_MSG_ (error_code, invalid_capacity_value, "Stack has invalid capacity");
        ERROR_MSG_ (error_code, overflow,               "Stack overflow has been occuried");
        ERROR_MSG_ (error_code, anti_overflow,          "Stack anti-overflow has been occuried");

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

    #define PRINT_VARIABLE_(variable, printf_specifier) do {\
                                                            fprintf_color (Console_green, Console_bold, stderr, "\t%s (%p) = ", #variable, &(variable));\
                                                            fprintf_color (Console_purple, Console_bold, stderr, printf_specifier "\n", variable);\
                                                        }while (0)

    ssize_t capacity = stack->capacity;
    ssize_t size = stack->size;

    PRINT_VARIABLE_ (size,     "%lu");
    PRINT_VARIABLE_ (capacity, "%lu");

    #undef PRINT_VARIABLE_

    fprintf_color (Console_purple, Console_normal, stderr, "\tdata (%p){\n ", stack->data);

    if (!stack->data){
        fprintf_color (Console_yellow, Console_bold, stderr, "\t\tUnable to read stack->data value\n}\n");
        fprintf_color (Console_red,    Console_bold, stderr, "}\n");

        return;
    }

    size_t real_stack_capacity = malloc_usable_size (const_cast <elem_t *> (stack->data)) / sizeof (elem_t);

    for (size_t index = 0; index < real_stack_capacity; index++){
        CONSOLE_COLOR data_color = Console_red;

        if ((ssize_t) index < stack->size)
            data_color = Console_green;

        fprintf_color (data_color,     Console_normal, stderr, "\t\tdata [%lu] (%p) = ", index, stack->data + index);
        fprintf_color (Console_purple, Console_normal, stderr, ElementPrintfSpecifier "\n", stack->data [index]);

    }

    fprintf_color (Console_purple, Console_bold, stderr, "\t}\n");

    fprintf_color (Console_red,   Console_bold, stderr, "}\n");
}

void StackDump (const Stack *stack, const Stack_error_code error_code, const char *function, const char *file, int line) {
    custom_assert_without_logger (function != NULL, pointer_is_null,     (void)0);
    custom_assert_without_logger (file     != NULL, pointer_is_null,     (void)0);
    custom_assert_without_logger (function != file, not_enough_pointers, (void)0);

    DumpErrors (error_code, function, file, line);

    DumpStackData (stack);

    fprintf_color (Console_red, Console_bold, stderr, "\nBACKTRACE:\n");

    Show_stack_trace ();

}
