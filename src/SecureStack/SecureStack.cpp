#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/types.h>

#include "ColorConsole.h"
#include "CustomAssert.h"
#include "SecureStack/SecureStack.h"
#include "Stack/ProcParser.h"
#include "Stack/StackPrintf.h"
#include "Logger.h"

typedef StackErrorCode CallbackFunction_t ();

#define FunctionName "(\033[1;36m%s\033[0;37m)"

#define FindStack(stack) \
    do {                                                                            \
        stack = GetStackFromDescriptor (requestMemory->stackDescriptor);            \
        if (!stack) {                                                               \
            PrintOperationResult (stderr, OPERATION_PROCESS_ERROR, operationName);  \
            WriteResult (STACK_POINTER_NULL);                                       \
            RETURN STACK_POINTER_NULL;                                              \
        }                                                                           \
    }while (0)

#define OperationFail(operation, error)                                     \
    do {                                                                    \
        PrintOperationResult (stderr, OPERATION_FAILED, operation);         \
        WriteCommandResponse (OPERATION_FAILED);                            \
        RETURN error;                                                       \
    }while (0)

#define OperationSuccess(operation)                                         \
    do {                                                                    \
        PrintOperationResult (stderr, OPERATION_SUCCESS, operation);        \
        WriteCommandResponse (OPERATION_SUCCESS);                           \
        RETURN NO_ERRORS;                                                   \
    }while (0)

#define OperationProcessError(operation)                                    \
    do {                                                                    \
        PrintOperationResult (stderr, OPERATION_PROCESS_ERROR, operation);  \
        WriteCommandResponse (OPERATION_PROCESS_ERROR);                     \
        RETURN SECURITY_PROCESS_ERROR;                                      \
    }while (0)

static int SecurityProcessPid = -1;
static StackOperationRequest *requestMemory = NULL;

static size_t StackCount = 0;
static void *StackBackups = NULL;

//=====================================================================================================================================================================================
//=================================================================== CHILD PROCESS PROTOTYPES ========================================================================================
//=====================================================================================================================================================================================

StackOperationRequest *CreateSharedMemory (size_t size);

static StackErrorCode SecurityProcessDestruct ();
static StackErrorCode SecurityProcessBackupLoop ();

static StackErrorCode StackPopSecureProcess      ();
static StackErrorCode StackPushSecureProcess     ();
static StackErrorCode StackInitSecureProcess     ();
static StackErrorCode StackDestructSecureProcess ();
static StackErrorCode VerifyStackSecureProcess   ();

static Stack *GetStackFromDescriptor (int stackDescriptor);

static void WriteCommandResponse (StackCommandResponse response);
static void WriteResult (StackErrorCode errorCode);

static void PrintSecurityProcessInfo (FILE *stream, MessageType messageType, const char *message, ...);
static void PrintOperationResult (FILE *stream, StackCommandResponse response, const char *operationName);

//=====================================================================================================================================================================================
//=================================================================== PARENT PROCESS PROTOTYPES =======================================================================================
//=====================================================================================================================================================================================


static StackCommandResponse CallBackupOperation (int descriptor, StackCommand operation, elem_t argument, Stack *stack);
StackErrorCode ResponseToError (StackCommandResponse response);

static void EncryptStackAddress (Stack *stack, Stack *outPointer, int descriptor);
static Stack *DecryptStackAddress (Stack *stack, int *descriptor);


//=====================================================================================================================================================================================
//=================================================================== CHILD PROCESS PART ==============================================================================================
//=====================================================================================================================================================================================


StackOperationRequest *CreateSharedMemory (size_t size){
    PushLog (3);

    StackOperationRequest *memory = (StackOperationRequest *) mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    memory->stackDescriptor = -1;
    memory->response = OPERATION_FAILED;
    memory->command = UNKNOWN_COMMAND;

    RETURN memory;
}

StackErrorCode SecurityProcessInit (){
    PushLog (1);

    requestMemory = CreateSharedMemory (sizeof (StackOperationRequest));

    SecurityProcessPid = fork ();

    if (SecurityProcessPid < 0){
        PrintSecurityProcessInfo (stderr, ERROR_MESSAGE, "Error occuried while forking\n");

        RETURN SECURITY_PROCESS_ERROR;
    }else if (SecurityProcessPid == 0){

        StackBackups = calloc (MAX_STACKS_COUNT, sizeof (Stack));

        if (!StackBackups){
            RETURN SECURITY_PROCESS_ERROR;
        }

        SecurityProcessBackupLoop ();

        RETURN NO_ERRORS;
    }else{

        StackBackups = calloc (MAX_STACKS_COUNT, sizeof (Stack));

        RETURN NO_ERRORS;
    }

}


StackErrorCode SecurityProcessDestruct () {
    PushLog (1);

    PrintSecurityProcessInfo (stderr, INFO_MESSAGE, "Destroying security process...\n");

    for (size_t stackIndex = 0; stackIndex < StackCount; stackIndex++){
        StackDestruct (GetStackFromDescriptor ((int) stackIndex));
    }

    free (StackBackups);

    WriteCommandResponse (OPERATION_SUCCESS);

    if (munmap (requestMemory, sizeof (StackOperationRequest)) != 0){
        PrintSecurityProcessInfo (stderr, ERROR_MESSAGE, "Memory deallocation error\n");
    }

    PrintSecurityProcessInfo (stderr, INFO_MESSAGE, "Aborting security process...\n");

    exit (0);
    RETURN NO_ERRORS;
}


StackErrorCode SecurityProcessBackupLoop (){
    PushLog (1);

    #define COMMAND_(targetCommand, callback)                                                                           \
        if (targetCommand == requestMemory->command){                                                                   \
            callbackFunction = callback;                                                                                \
            PrintSecurityProcessInfo (stderr, INFO_MESSAGE, "Command \033[1;35m%s\033[0;37m found\n", #targetCommand);  \
        }

    do{
        if (requestMemory->response != OPERATION_PROCESSING)
            continue;

        CallbackFunction_t *callbackFunction = NULL;

        COMMAND_ (STACK_PUSH_COMMAND,     StackPushSecureProcess     );
        COMMAND_ (STACK_POP_COMMAND,      StackPopSecureProcess      );
        COMMAND_ (STACK_INIT_COMMAND,     StackInitSecureProcess     );
        COMMAND_ (STACK_DESTRUCT_COMMAND, StackDestructSecureProcess );
        COMMAND_ (STACK_VERIFY_COMMAND,   VerifyStackSecureProcess   );
        COMMAND_ (ABORT_PROCESS,          SecurityProcessDestruct    );

        if (requestMemory->command == UNKNOWN_COMMAND){
            PrintSecurityProcessInfo (stderr, WARNING_MESSAGE, "Something has gone wrong\n");
            WriteCommandResponse (OPERATION_PROCESS_ERROR);
        }

        StackErrorCode errorCode = NO_ERRORS;

        if (callbackFunction == NULL)
            continue;

        if ((errorCode = callbackFunction ()) != NO_ERRORS){
            PrintSecurityProcessInfo (stderr, WARNING_MESSAGE, "Stack error has been occuried!");

        }

        callbackFunction = NULL;

    }while (requestMemory->command != ABORT_PROCESS);

    exit (0);

    #undef COMMAND_

    RETURN NO_ERRORS;
}

StackErrorCode StackInitSecureProcess () {
    PushLog (2);

    const char operationName [] = "init stack";

    if (StackCount >= MAX_STACKS_COUNT){
        OperationProcessError (operationName);
    }

    StackErrorCode errorCode = StackInit ((Stack *) StackBackups + StackCount, {"", "", 0, "", false});

    if (errorCode == NO_ERRORS) {
        requestMemory->stackDescriptor = (int) StackCount;
        StackCount++;

        OperationSuccess (operationName);
    }

    OperationFail (operationName, errorCode);
}


StackErrorCode StackDestructSecureProcess () {
    PushLog (2);

    const char operationName [] = "stack destruct";

    Stack *stack = NULL;
    FindStack (stack);

    StackErrorCode errorCode = StackDestruct (stack);

    if (errorCode == NO_ERRORS){
        OperationSuccess (operationName);
    }

    OperationFail (operationName, errorCode);
}


StackErrorCode StackPopSecureProcess () {
    PushLog (2);

    const char operationName [] = "stack pop";

    Stack *stack = NULL;
    FindStack (stack);

    StackErrorCode errorCode =  StackPop (stack, NULL, {"","", 0, "", false});

    if (errorCode == NO_ERRORS){
        OperationSuccess (operationName);
    }

    OperationFail (operationName, errorCode);
}


StackErrorCode StackPushSecureProcess () {
    PushLog (2);

    const char operationName [] = "stack push";

    Stack *stack = NULL;
    FindStack (stack);

    StackErrorCode errorCode = StackPush (stack, requestMemory->argument, {"","", 0, "", false});

    if (errorCode == NO_ERRORS) {
        PrintSecurityProcessInfo (stderr, SUCCESS_MESSAGE , "Command executed " FunctionName ". Pushed ", operationName);
        PrintData (CONSOLE_WHITE, CONSOLE_NORMAL, stderr, requestMemory->argument);
        fprintf (stderr, "\n");

        WriteResult (NO_ERRORS);
        RETURN NO_ERRORS;
    }

    OperationFail (operationName, errorCode);
}

StackErrorCode VerifyStackSecureProcess (){
    PushLog (2);

    const char operationName [] = "stack verification";

    Stack *stack = NULL;
    FindStack (stack);

    if (requestMemory->dataHash == stack->dataHash){
        OperationSuccess (operationName);
    }

    OperationFail (operationName, WRONG_DATA_HASH);
}

Stack *GetStackFromDescriptor (int stackDescriptor){
    PushLog (3);

    custom_assert (stackDescriptor >= 0,                  invalid_value, NULL);
    custom_assert ((size_t) stackDescriptor < StackCount, invalid_value, NULL);

    RETURN (Stack *) StackBackups + stackDescriptor;
}

void WriteResult (StackErrorCode errorCode) {
    PushLog (3);


    if (errorCode == NO_ERRORS){
        WriteCommandResponse (OPERATION_SUCCESS);

        RETURN;
    }

    WriteCommandResponse (OPERATION_FAILED);
    RETURN;
}

void WriteCommandResponse (StackCommandResponse response) {
    PushLog (3);
    custom_assert (IsAddressValid (requestMemory), pointer_is_null, (void)0);

    requestMemory->response = response;

    RETURN;
}

#ifndef _NDEBUG

static void PrintSecurityProcessInfo (FILE *stream, MessageType messageType, const char *message, ...) {
    PushLog (3);
    custom_assert (stream,  pointer_is_null, (void)0);
    custom_assert (message, pointer_is_null, (void)0);

    va_list args = {};
    va_start (args, message);

    CONSOLE_COLOR color = CONSOLE_DEFAULT;

    switch (messageType){
        case INFO_MESSAGE:
            color = CONSOLE_BLUE;
            break;

        case ERROR_MESSAGE:
            color = CONSOLE_RED;
            break;

        case WARNING_MESSAGE:
            color = CONSOLE_YELLOW;
            break;

        case SUCCESS_MESSAGE:
            color = CONSOLE_GREEN;
            break;

        default:
            color = CONSOLE_DEFAULT;
            break;
    };

    fprintf_color  (color,         CONSOLE_BOLD,   stream, "[SECURITY PROCESS]: ");
    vfprintf_color (CONSOLE_WHITE, CONSOLE_NORMAL, stream, message, args);

    va_end (args);

    RETURN;
}

static void PrintOperationResult (FILE *stream, StackCommandResponse response, const char *operationName) {
    PushLog (3);
    custom_assert (stream,        pointer_is_null, (void)0);
    custom_assert (operationName, pointer_is_null, (void)0);

    char *formatString = NULL;
    MessageType type = INFO_MESSAGE;

    #define MESSAGE_(operation, message, messageType)       \
            case operation:                                 \
                formatString = message FunctionName "\n";   \
                type = messageType;                         \
                break;

    switch (response){

        MESSAGE_ (OPERATION_FAILED,        "Stack corruption has been detected ", WARNING_MESSAGE );
        MESSAGE_ (OPERATION_PROCESSING,    "Operation is still processing now ",  INFO_MESSAGE    );
        MESSAGE_ (OPERATION_SUCCESS,       "Command executed ",                   SUCCESS_MESSAGE );
        MESSAGE_ (OPERATION_PROCESS_ERROR, "Error occuried ",                     ERROR_MESSAGE   );

        default:
            RETURN;
            break;
    };

    #undef MESSAGE_

    PrintSecurityProcessInfo (stream, type, formatString, operationName);

    RETURN;
}
#else

static void PrintSecurityProcessInfo (FILE *stream, MessageType messageType, char *message, ...) {}
static void PrintOperationResult (FILE *stream, StackCommandResponse response, char *operationName) {}

#endif

#undef FunctionName
#undef FindStack
#undef OperationFail
#undef OperationSuccess
#undef OperationProcessError

//=====================================================================================================================================================================================
//=================================================================== PARENT PROCESS PART =============================================================================================
//=====================================================================================================================================================================================


#define WriteError(errorStack, function) (errorStack)->errorCode = (StackErrorCode) (ResponseToError (function) | (errorStack)->errorCode)

StackErrorCode SecurityProcessStop () {
    PushLog (2);

    CallBackupOperation (-1, ABORT_PROCESS, 0, NULL);

    fprintf (stderr, "Waiting for security process %d to stop\n", SecurityProcessPid);

    wait (&SecurityProcessPid);

    fprintf (stderr, "Wait passed\n");
    RETURN NO_ERRORS;
}


StackErrorCode StackInitSecure (Stack *stack, const StackCallData callData, ssize_t initialCapacity) {
    PushLog (2);

    custom_assert (IsAddressValid (stack), pointer_is_null, STACK_POINTER_NULL);

    int descriptor = -1;

    StackInit ((Stack *) StackBackups + StackCount, callData, initialCapacity);
    StackCommandResponse response = CallBackupOperation (descriptor, STACK_INIT_COMMAND, 0, (Stack *)StackBackups + StackCount);

    if (response == OPERATION_SUCCESS){
        descriptor = requestMemory->stackDescriptor;
    }else{
        WriteError ((Stack *)StackBackups + StackCount, response);
    }

    WriteError ((Stack *)StackBackups + StackCount, CallBackupOperation (descriptor, STACK_VERIFY_COMMAND, 0, (Stack *)StackBackups + StackCount));

    EncryptStackAddress ((Stack *)StackBackups + StackCount, stack, descriptor);

    RETURN ((Stack *)StackBackups + StackCount)->errorCode;
}


StackErrorCode StackDestructSecure (Stack *stack){
    PushLog (2);

    custom_assert (IsAddressValid (stack), pointer_is_null, STACK_POINTER_NULL);

    int descriptor = -1;

    Stack *realStack = DecryptStackAddress (stack, &descriptor);

    StackErrorCode errorCode = StackDestruct (realStack);
    errorCode = (StackErrorCode) (ResponseToError (CallBackupOperation (descriptor, STACK_DESTRUCT_COMMAND, 0, realStack)) | errorCode);

    memset (stack, 0, sizeof (Stack));

    RETURN errorCode;
}


StackErrorCode StackPopSecure (Stack *stack, elem_t *RETURNValue, const StackCallData callData) {
    PushLog (2);

    custom_assert (stack, pointer_is_null, STACK_POINTER_NULL);

    int descriptor = -1;

    Stack *realStack = DecryptStackAddress (stack, &descriptor);

    WriteError (realStack, CallBackupOperation (descriptor, STACK_VERIFY_COMMAND, 0, realStack));

    StackPop (realStack, RETURNValue, callData);
    WriteError (realStack, CallBackupOperation (descriptor, STACK_POP_COMMAND,    0, realStack));

    WriteError (realStack, CallBackupOperation (descriptor, STACK_VERIFY_COMMAND, 0, realStack));

    RETURN realStack->errorCode;
}


StackErrorCode StackPushSecure (Stack *stack, elem_t value, const StackCallData callData) {
    PushLog (2);

    custom_assert (stack, pointer_is_null, STACK_POINTER_NULL);

    int descriptor = -1;

    Stack *realStack = DecryptStackAddress (stack, &descriptor);

    WriteError (realStack, CallBackupOperation (descriptor, STACK_VERIFY_COMMAND, 0,     realStack));

    StackPush (realStack, value, callData);
    WriteError (realStack, CallBackupOperation (descriptor, STACK_PUSH_COMMAND,   value, realStack));

    WriteError (realStack, CallBackupOperation (descriptor, STACK_VERIFY_COMMAND, 0,     realStack));

    RETURN realStack->errorCode;
}

StackCommandResponse CallBackupOperation (int descriptor, StackCommand operation, elem_t argument, Stack *stack){
    PushLog (3);

    requestMemory->stackDescriptor = descriptor;
    requestMemory->argument = argument;
    requestMemory->command = operation;

    if (stack != NULL){
        requestMemory->stackHash = stack->stackHash;
        requestMemory->dataHash  = stack->dataHash;
    }

    requestMemory->response = OPERATION_PROCESSING;

    while (requestMemory->response == OPERATION_PROCESSING && operation != ABORT_PROCESS);

    RETURN requestMemory->response;
}

StackErrorCode ResponseToError (StackCommandResponse response){
    return (response == OPERATION_SUCCESS ? NO_ERRORS : EXTERNAL_VERIFY_FAILED);
}


//===================================================================================================================================================================
// My first intension was to to obfuscate this code below |, but I'm too lazy :) Just don't copy (and watch) it pleaaase=============================================
//=======================================================\ /=========================================================================================================
//==============================================================!!!WARNING!!!========================================================================================
//==================================== CODE BELOW WORKS ONLY WITH 8 BITS IN BYTE, sizeof (char) == 1 AND sizeof (Stack *) == 32 =====================================
//===================================================================================================================================================================

Stack *DecryptStackAddress (Stack *stack, int *descriptor) {
    // Why are you doing that, dude?

    PushLog (4);

    custom_assert(stack,      pointer_is_null, NULL);
    custom_assert(descriptor, pointer_is_null, NULL);

    char *stackChar = (char *) stack;
    Stack *RETURNPointer = 0;
    char * RETURNPointerPointer = (char *) (&RETURNPointer);
    size_t byteNumber = 0;

    int checksum = 0;

    // Seriously, you have to stop

    const size_t worksOnlyForThisBitsInChar = 8;

    for (size_t byteIndex = 0; byteIndex < 4; byteIndex++){
        for (size_t bit = 0; bit < worksOnlyForThisBitsInChar; bit++){
            if (stackChar [byteIndex] & (1 << bit)){

                if (byteIndex == 0){
                    RETURN NULL;
                }

                if (byteNumber >= sizeof (Stack *)){
                    RETURN NULL;
                }

                checksum += stackChar [byteIndex * 8 + bit];

                RETURNPointerPointer [byteNumber++] = stackChar [byteIndex * 8 + bit];
            }
        }
    }

    if (checksum != *((int *) (void *) (stackChar + 4))){
        RETURN NULL;
    }

    *descriptor = *((int *) (void *) (stackChar + 3 * worksOnlyForThisBitsInChar));

    // It would be unfair if u copy that :(

    RETURN RETURNPointer;
}

void EncryptStackAddress (Stack *stack, Stack *outPointer, int descriptor) {
    PushLog (4);

    custom_assert (stack,               pointer_is_null,     (void) 0);
    custom_assert (outPointer,          pointer_is_null,     (void) 0);
    custom_assert (outPointer != stack, not_enough_pointers, (void) 0);

    char *outPointerChar = (char *) outPointer;
    char *stackChar      = (char *) &stack;

    srand ((unsigned int) time (NULL));

    int checksum = 0;

    memset (outPointerChar, 0, 8);

    const size_t worksOnlyForThisBitsInChar = 8;

    for (size_t pointerByte = 0; pointerByte < sizeof (Stack *); pointerByte++){
        size_t pointerPosition = 0, addressByte = 0, addressBit = 0;
        do {
            pointerPosition = (size_t) rand () % 2 + pointerByte * 2 + sizeof (int) + sizeof (Stack) / worksOnlyForThisBitsInChar;

            addressByte = pointerPosition / worksOnlyForThisBitsInChar;
            addressBit  = pointerPosition % worksOnlyForThisBitsInChar;
        } while (outPointerChar [addressByte] & 1 << addressBit);

        outPointerChar [pointerPosition] = stackChar [pointerByte];

        checksum += stackChar [pointerByte];

        outPointerChar [addressByte] |= (char) (1 << addressBit);
    }

    *((int *) (void *) (outPointerChar + 4)) = checksum;

    *((int *) (void *) (outPointerChar + 3 * worksOnlyForThisBitsInChar)) = descriptor;

    RETURN;
}

#undef WriteError
