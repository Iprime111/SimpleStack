#include <cstddef>
#include <cstring>
#include <malloc.h>

#include "Logger.h"
#include "Stack/Stack.h"
#include "Stack/StackHash.h"
#include "Stack/StackPrintf.h"
#include "Stack/ProcParser.h"
#include "CustomAssert.h"
#include "ColorConsole.h"

#define DataMemorySize(stack) ((size_t)(stack)->capacity * sizeof (elem_t) ON_USE_CANARY (+ 2 * sizeof (canary_t)))

static StackErrorCode StackRealloc (Stack *stack, const StackCallData callData);
static StackErrorCode StackVerifier (Stack *stack);

static void DumpErrors (const StackErrorCode errorCode, const char *function, const char *file, int line, const StackCallData callData);

static size_t getRealCapacity (const Stack *stack);
static size_t getRealAllocSize (const Stack *stack);

static void UpdateHashes (Stack *stack, size_t dataSize);
static void CorrectAlignment (Stack *stack);

#ifdef _USE_HASH


    static void UpdateHashes (Stack *stack, size_t dataSize) {
        StackErrorCode hashErrorCode = NO_ERRORS;
        hashErrorCode = (StackErrorCode) (hashErrorCode | ComputeStackHash (stack, &((stack)->stackHash)));
        hashErrorCode = (StackErrorCode) (hashErrorCode | ComputeDataHash  (stack, dataSize, &((stack)->dataHash)));
        (stack)->errorCode = (StackErrorCode) (hashErrorCode | (stack)->errorCode);
    }

#else

    static void UpdateHashes (Stack *stack, size_t dataSize) {}

#endif


StackErrorCode StackInit (Stack *stack, const StackCallData callData, ssize_t initialCapacity) {
    PushLog (2);

    custom_assert (initialCapacity > 0, invalid_value, INVALID_INPUT);
    custom_assert (stack, pointer_is_null, INVALID_INPUT);

    StackDestruct (stack);

    stack->capacity = initialCapacity;
    stack->size = 0;
    stack->errorCode = NO_ERRORS;

    CorrectAlignment (stack);
    size_t callocSize =  (size_t) stack->capacity * sizeof (elem_t) ON_USE_CANARY (+ sizeof (canary_t) * 2);

    #ifdef _USE_CANARY
        stack->rightCanary = CanaryNormal;
        stack->leftCanary = CanaryNormal;
    #endif

    custom_assert (callocSize < MaxAllocSize, allocation_error, OVERFLOW);

    stack->data = (elem_t *) calloc (1, callocSize);

    #ifdef _USE_CANARY
        stack->data = (elem_t *) ((canary_t *) stack->data + 1);

        *leftCanaryPointer (stack)  = CanaryNormal;
        *rightCanaryPointer (stack) = CanaryNormal;
    #endif

    #ifdef _USE_HASH
        stack->stackHash = 0;
        stack->dataHash = 0;
    #endif

    UpdateHashes (stack, callocSize);

    VerifyStack (stack, callData);

    RETURN NO_ERRORS;
}


StackErrorCode StackDestruct (Stack *stack) {
    PushLog (2);

    if (!IsAddressValid (stack)){
        RETURN STACK_POINTER_NULL;
    }

    if (IsAddressValid (stack->data)) {

        memset(leftCanaryPointer (stack), 0, getRealAllocSize (stack));
        free  (leftCanaryPointer (stack));
    }

    stack->data = NULL;
    stack->capacity = -1;
    stack->size = -1;
    stack->errorCode = (StackErrorCode) (STACK_POINTER_NULL | DATA_POINTER_NULL | INVALID_CAPACITY_VALUE | ANTI_OVERFLOW | OVERFLOW | INVALID_INPUT | REALLOCATION_ERROR | STACK_CANARY_CORRUPTED);

    RETURN NO_ERRORS;
}


static StackErrorCode StackRealloc (Stack *stack, const StackCallData callData){
    PushLog (3);
    VerifyStack (stack, callData);

    if (stack->size == stack->capacity){
        stack->capacity *= ReallocationScale;

    }else if (stack->size < stack->capacity / (ReallocationScale * ReallocationScale)){
        stack->capacity /= ReallocationScale;

    }else{
        RETURN NO_ERRORS;
    }

   CorrectAlignment (stack);

    size_t reallocSize  = (size_t) stack->capacity * sizeof (elem_t) ON_USE_CANARY (+ sizeof (canary_t) * 2);


    elem_t *testDataPointer = (elem_t *) realloc (leftCanaryPointer (stack),  reallocSize);

    if (!testDataPointer){
        stack->errorCode = (StackErrorCode) (stack->errorCode | REALLOCATION_ERROR);
        RETURN stack->errorCode;
    }

    ON_USE_CANARY (testDataPointer = (elem_t *) ((canary_t *) testDataPointer + 1));

    stack->data = testDataPointer;

    ON_USE_CANARY (*rightCanaryPointer (stack) = CanaryNormal);

    UpdateHashes (stack, reallocSize);

    VerifyStack (stack, callData);

    RETURN stack->errorCode;
}


StackErrorCode StackPop (Stack *stack, elem_t *returnValue, const StackCallData callData) {
    PushLog (2);

    VerifyStack (stack, callData);

    if (stack->size == 0){
        stack->errorCode = (StackErrorCode) (stack->errorCode | ANTI_OVERFLOW);

        DumpStack (stack, callData);

        RETURN stack->errorCode;
    }

    stack->size--;

    if (returnValue){
        *returnValue = stack->data [stack->size];
    }

    UpdateHashes (stack, DataMemorySize(stack));

    stack->errorCode = (StackErrorCode) (StackRealloc (stack, callData) | stack->errorCode);

    RETURN stack->errorCode;
}


StackErrorCode StackPush (Stack *stack, elem_t value, const StackCallData callData) {
    PushLog (2);
    VerifyStack (stack, callData);

    stack->errorCode = (StackErrorCode) (stack->errorCode | StackRealloc (stack, callData));

    if (stack->errorCode == NO_ERRORS)
        stack->data [stack->size++] = value;

    UpdateHashes (stack, DataMemorySize(stack));

    RETURN stack->errorCode;
}

static StackErrorCode StackVerifier (Stack *stack) {
    PushLog (3);

    if (!IsAddressValid (stack)){
        RETURN STACK_POINTER_NULL;
    }

    #define VerifyExpression_(errorExp, patternErrorCode)                                   \
        if (!(errorExp)) {                                                                  \
            (stack->errorCode) = (StackErrorCode) ((stack->errorCode) | patternErrorCode);  \
        }

    VerifyExpression_ (IsAddressValid(stack->data),        DATA_POINTER_NULL);
    VerifyExpression_ (stack->capacity >= 0,               INVALID_CAPACITY_VALUE);
    VerifyExpression_ (stack->size >= 0,                   ANTI_OVERFLOW);
    VerifyExpression_ (stack->size <= stack->capacity,     OVERFLOW);

    #ifdef _USE_CANARY
        VerifyExpression_ (stack->leftCanary == CanaryNormal,  STACK_CANARY_CORRUPTED);
        VerifyExpression_ (stack->rightCanary == CanaryNormal, STACK_CANARY_CORRUPTED);


        if (!(stack->errorCode & DATA_POINTER_NULL)){
            VerifyExpression_ (*leftCanaryPointer(stack) == CanaryNormal,     DATA_CANARY_CORRUPTED);
            VerifyExpression_ (*rightCanaryPointer(stack) == CanaryNormal,    DATA_CANARY_CORRUPTED);
        }
    #endif

    #ifdef _USE_HASH
        size_t allocSize = DataMemorySize (stack);

        StackErrorCode stackHashErrorCode = CompareStackHash (stack);
        StackErrorCode  dataHashErrorCode = CompareDataHash  (stack, allocSize);

        VerifyExpression_(stackHashErrorCode == NO_ERRORS,  stackHashErrorCode);
        VerifyExpression_( dataHashErrorCode == NO_ERRORS,  dataHashErrorCode);

    #endif

    #undef VerifyExpression_

    RETURN stack->errorCode;
}

static void DumpErrors (const StackErrorCode errorCode, const char *function, const char *file, int line, const StackCallData callData){
    custom_assert_without_logger (function != NULL, pointer_is_null,     (void)0);
    custom_assert_without_logger (file     != NULL, pointer_is_null,     (void)0);
    custom_assert_without_logger (function != file, not_enough_pointers, (void)0);

    fprintf_color (CONSOLE_RED, CONSOLE_NORMAL, stderr, "Stack has been corrupted in function %s (%s:%d)\n", function, file, line);
    fprintf_color (CONSOLE_RED, CONSOLE_NORMAL, stderr, "Stack variable name: \"%s\".Called from %s (%s:%d)\n", callData.variableName, callData.function, callData.file, callData.line);

    fprintf_color (CONSOLE_RED, CONSOLE_BOLD, stderr, "REPORT:\n");

    if (errorCode == NO_ERRORS){
        fprintf_color (CONSOLE_WHITE, CONSOLE_BOLD, stderr, "No errors have been registered\n");
    }else{
        #define ERROR_MSG_(error, errorPattern, message)                                                \
            if (error & errorPattern){                                                                  \
                fprintf_color (CONSOLE_WHITE, CONSOLE_BOLD, stderr, "%s: %s\n", #errorPattern, message);\
            }

        ERROR_MSG_ (errorCode, STACK_POINTER_NULL,     "Pointer to stack variable has NULL value");
        ERROR_MSG_ (errorCode, DATA_POINTER_NULL,      "Pointer to stack data has NULL value");
        ERROR_MSG_ (errorCode, INVALID_CAPACITY_VALUE, "Stack has invalid capacity");
        ERROR_MSG_ (errorCode, OVERFLOW,               "Stack overflow has been occuried");
        ERROR_MSG_ (errorCode, ANTI_OVERFLOW,          "Stack anti-overflow has been occuried");
        ERROR_MSG_ (errorCode, REALLOCATION_ERROR,     "Memory reallocation failed");
        ERROR_MSG_ (errorCode, STACK_CANARY_CORRUPTED, "Stack canary has been corrupted");
        ERROR_MSG_ (errorCode, DATA_CANARY_CORRUPTED,  "Data canary has been corrupted");
        ERROR_MSG_ (errorCode, WRONG_STACK_HASH,       "Stack has been corrupted");
        ERROR_MSG_ (errorCode, WRONG_DATA_HASH,        "Data has been corrupted");
        ERROR_MSG_ (errorCode, EXTERNAL_VERIFY_FAILED, "External verify has been failed");

        #undef  ERROR_MSG_
    }

    fputs ("\n", stderr);
}


void DumpStackData (const Stack *stack) {
    if (!IsAddressValid (stack)){
        fprintf_color (CONSOLE_RED, CONSOLE_BOLD, stderr, "Unable to read stack value\n");

        return;
    }

    fprintf_color (CONSOLE_RED, CONSOLE_BOLD, stderr, "Stack (%p){\n", stack);

    #define PRINT_VARIABLE_(variable, printfSpecifier)                                                      \
        do {                                                                                                \
            fprintf_color (CONSOLE_GREEN,  CONSOLE_BOLD, stderr, "\t%s (%p) = ", #variable, &(variable));   \
            fprintf_color (CONSOLE_PURPLE, CONSOLE_BOLD, stderr, printfSpecifier "\n", variable);           \
        }while (0)

    size_t realStackCapacity = getRealCapacity (stack);

    PRINT_VARIABLE_ (stack->size,              "%lld");
    PRINT_VARIABLE_ (stack->capacity,          "%lld");
    PRINT_VARIABLE_ (realStackCapacity,        "%lu");

    #ifdef _USE_HASH
        PRINT_VARIABLE_(stack->stackHash, "%llu");
        PRINT_VARIABLE_(stack->dataHash,  "%llu");
    #endif

    #ifdef _USE_CANARY
        PRINT_VARIABLE_ (stack->leftCanary,        "%x");
        PRINT_VARIABLE_ (stack->rightCanary,       "%x");
    #endif

    #undef PRINT_VARIABLE_


    if (!IsAddressValid (stack->data)) {
        fprintf_color (CONSOLE_YELLOW, CONSOLE_BOLD, stderr, "\tdata (%p){\n\t\tUnable to read stack->data value\n\t}\n", stack->data);
        fprintf_color (CONSOLE_RED,    CONSOLE_BOLD, stderr, "}\n");

        return;
    }

    #ifdef _USE_CANARY
        fprintf_color (CONSOLE_GREEN,  CONSOLE_BOLD, stderr, "\t%s (%p) = ", "leftDataCanary", leftCanaryPointer (stack));
        fprintf_color (CONSOLE_PURPLE, CONSOLE_BOLD, stderr, "%x\n", *leftCanaryPointer (stack));

        fprintf_color (CONSOLE_GREEN,  CONSOLE_BOLD, stderr, "\t%s (%p) = ", "rightDataCanary", ((canary_t *) (stack->data + realStackCapacity)));
        fprintf_color (CONSOLE_PURPLE, CONSOLE_BOLD, stderr, "%x\n", *((canary_t *) (stack->data +realStackCapacity)));
    #endif

    fprintf_color (CONSOLE_PURPLE, CONSOLE_NORMAL, stderr, "\tdata (%p){\n ", stack->data);

    for (size_t index = 0; index < realStackCapacity; index++){
        CONSOLE_COLOR dataColor = CONSOLE_RED;

        if ((ssize_t) index < stack->size)
            dataColor = CONSOLE_GREEN;

        fprintf_color (dataColor,     CONSOLE_NORMAL, stderr, "\t\tdata [%lu] (%p) = ", index, stack->data + index);

        PrintData (CONSOLE_PURPLE, CONSOLE_NORMAL, stderr, stack->data [index]);

        fprintf_color (CONSOLE_PURPLE, CONSOLE_NORMAL, stderr, "\n");

    }

    fprintf_color (CONSOLE_PURPLE, CONSOLE_BOLD, stderr, "\t}\n");

    fprintf_color (CONSOLE_RED,   CONSOLE_BOLD, stderr, "}\n");
}


void StackDump (const Stack *stack, const char *function, const char *file, int line, const StackCallData callData) {
    custom_assert_without_logger (function != NULL, pointer_is_null,     (void)0);
    custom_assert_without_logger (file     != NULL, pointer_is_null,     (void)0);
    custom_assert_without_logger (function != file, not_enough_pointers, (void)0);

    if (!callData.showDump)
        return;

    DumpErrors ((IsAddressValid (stack) ? stack->errorCode : STACK_POINTER_NULL), function, file, line, callData);

    DumpStackData (stack);

    fprintf_color (CONSOLE_RED, CONSOLE_BOLD, stderr, "\nBACKTRACE:\n");

    Show_stack_trace ();
}


static size_t getRealAllocSize (const Stack *stack){
    PushLog (3);

    if (!IsAddressValid (stack) || !IsAddressValid (stack->data)){
        RETURN 0;
    }

    RETURN malloc_usable_size (leftCanaryPointer (stack));
}


static size_t getRealCapacity (const Stack *stack){
    PushLog (3);

    if (!IsAddressValid (stack) || !IsAddressValid (stack->data)){
        RETURN 0;
    }


    RETURN (getRealAllocSize(stack) ON_USE_CANARY (- 2 * sizeof (canary_t))) / sizeof (elem_t);
}

static void CorrectAlignment (Stack *stack){
    PushLog (3);
    #ifdef _USE_CANARY
        custom_assert (IsAddressValid (stack), pointer_is_null, (void) 0);

        while (((size_t) stack->capacity * sizeof (elem_t)) % sizeof (canary_t) != 0)
            stack->capacity++;

    #endif

    RETURN;
}

void PrintStackLogo (FILE *stream) {
    PushLog (3);

    if (!IsAddressValid (stream)){
        RETURN;
    }

    fputs ("\n\n", stream);

    char *logo [] = {


        "  /$$$$$$  /$$                         /$$                  /$$$$$$   /$$                         /$$",
        " /$$__  $$|__/                        | $$                 /$$__  $$ | $$                        | $$",
        "| $$  \\__/ /$$ /$$$$$$/$$$$   /$$$$$$ | $$  /$$$$$$       | $$  \\__//$$$$$$    /$$$$$$   /$$$$$$$| $$   /$$",
        "|  $$$$$$ | $$| $$_  $$_  $$ /$$__  $$| $$ /$$__  $$      |  $$$$$$|_  $$_/   |____  $$ /$$_____/| $$  /$$/",
        " \\____  $$| $$| $$ \\ $$ \\ $$| $$  \\ $$| $$| $$$$$$$$       \\____  $$ | $$      /$$$$$$$| $$      | $$$$$$/",
        " /$$  \\ $$| $$| $$ | $$ | $$| $$  | $$| $$| $$_____/       /$$  \\ $$ | $$ /$$ /$$__  $$| $$      | $$_  $$",
        "|  $$$$$$/| $$| $$ | $$ | $$| $$$$$$$/| $$|  $$$$$$$      |  $$$$$$/ |  $$$$/|  $$$$$$$|  $$$$$$$| $$ \\  $$",
        " \\______/ |__/|__/ |__/ |__/| $$____/ |__/ \\_______/       \\______/   \\___/   \\_______/ \\_______/|__/  \\__/",
        "                            | $$",
        "                            | $$",
        "                            |__/"
    };

    for (size_t line = 0; line < sizeof (logo) / sizeof (logo [0]); line++){
        fputs (logo [line], stream);
        fputs ("\n", stream);
    }

    fputs ("\n\nCopyright Ded's course, MIPT, 2023\n\n\n\n", stream);

    fflush (stream);

    RETURN;
}
