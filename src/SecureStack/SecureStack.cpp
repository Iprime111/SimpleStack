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
#include "Stack/StackPrintf.h"
#include "Logger.h"

typedef StackErrorCode CallbackFunction_t ();

static const unsigned int CycleSleepTime = 1;

static int SecurityProcessPid = -1;
static StackOperationRequest *requestMemory = NULL;

static size_t StackCount = 0;
static void *StackBackups = NULL;

//=====================================================================================================================================================================================
//====================================================================CHILD PROCESS PROTOTYPES=========================================================================================
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

static void PrintSecurityProcessInfo (FILE *stream, MessageType messageType, char *message, ...);
static void PrintOperationResult (FILE *stream, StackCommandResponse response,char *operationName);

//=====================================================================================================================================================================================
//====================================================================PARENT PROCESS PROTOTYPES========================================================================================
//=====================================================================================================================================================================================


static StackCommandResponse CallBackupOperation (int descriptor, StackCommand operation, elem_t argument, Stack *stack);
StackErrorCode ResponseToError (StackCommandResponse response);

static void EncryptStackAddress (Stack *stack, Stack *outPointer, int descriptor);
static Stack *DecryptStackAddress (Stack *stack, int *descriptor);


//=====================================================================================================================================================================================
//====================================================================CHILD PROCESS PART===============================================================================================
//=====================================================================================================================================================================================


StackOperationRequest *CreateSharedMemory (size_t size){
    StackOperationRequest *memory = (StackOperationRequest *) mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    memory->stackDescriptor = -1;
    memory->response = OPERATION_FAILED;
    memory->command = UNKNOWN_COMMAND;

    return memory;
}

StackErrorCode SecurityProcessInit (){
    requestMemory = CreateSharedMemory (sizeof (StackOperationRequest));

    SecurityProcessPid = fork ();

    if (SecurityProcessPid < 0){
        PrintSecurityProcessInfo (stderr, ERROR_MESSAGE, "Error occuried while forking\n");

        return SECURITY_PROCESS_ERROR;
    }else if (SecurityProcessPid == 0){

        StackBackups = calloc (MAX_STACKS_COUNT, sizeof (Stack));

        if (!StackBackups){
            return SECURITY_PROCESS_ERROR;
        }

        SecurityProcessBackupLoop ();

        return NO_ERRORS;
    }else{

        StackBackups = calloc (MAX_STACKS_COUNT, sizeof (Stack));

        return NO_ERRORS;
    }

}


StackErrorCode SecurityProcessDestruct () {
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
    return NO_ERRORS;
}


StackErrorCode SecurityProcessBackupLoop (){

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
            return errorCode;
        }else {
            callbackFunction = NULL;
        }

        usleep (CycleSleepTime);

    }while (requestMemory->command != ABORT_PROCESS);

    exit (0);

    #undef COMMAND_

    return NO_ERRORS;
}

StackErrorCode StackInitSecureProcess () {
    char operationName [] = "init stack";

    if (StackCount >= MAX_STACKS_COUNT){
        WriteCommandResponse (OPERATION_PROCESS_ERROR);
        PrintOperationResult (stderr, OPERATION_PROCESS_ERROR, operationName);

        return SECURITY_PROCESS_ERROR;
    }

    StackErrorCode errorCode = StackInit ((Stack *) StackBackups + StackCount, {"", "", 0, "", false});

    if (errorCode == NO_ERRORS) {
        requestMemory->stackDescriptor = (int) StackCount;
        StackCount++;

        WriteCommandResponse (OPERATION_SUCCESS);
        PrintOperationResult (stderr, OPERATION_SUCCESS, operationName);
    }else {
        WriteCommandResponse (OPERATION_FAILED);
        PrintOperationResult (stderr, OPERATION_FAILED, operationName);
    }

    return errorCode;
}


StackErrorCode StackDestructSecureProcess () {
    char operationName [] = "stack destruct";

    StackErrorCode errorCode = StackDestruct (GetStackFromDescriptor (requestMemory->stackDescriptor));

    WriteResult (errorCode);
    PrintOperationResult (stderr, OPERATION_SUCCESS, operationName);

    return errorCode;
}


StackErrorCode StackPopSecureProcess () {
    char operationName [] = "stack pop";

    StackErrorCode errorCode =  StackPop (GetStackFromDescriptor (requestMemory->stackDescriptor), NULL, {"","", 0, "", false});

    WriteResult (errorCode);
    PrintOperationResult (stderr, OPERATION_SUCCESS, operationName);

    return errorCode;
}


StackErrorCode StackPushSecureProcess () {
    char operationName [] = "stack push";

    StackErrorCode errorCode = StackPush (GetStackFromDescriptor (requestMemory->stackDescriptor), requestMemory->argument, {"","", 0, "", false});

    WriteResult (errorCode);

    PrintSecurityProcessInfo (stderr,SUCCESS_MESSAGE , "Command executed (\033[1;36m%s\033[0;37m). Pushed ", operationName);
    print_data (CONSOLE_WHITE, CONSOLE_NORMAL, stderr, requestMemory->argument);
    fprintf (stderr, "\n");

    return errorCode;
}

StackErrorCode VerifyStackSecureProcess (){
    char operationName [] = "stack verification";

    Stack *stack = GetStackFromDescriptor (requestMemory->stackDescriptor);

    if (stack == NULL){
        WriteCommandResponse (OPERATION_PROCESS_ERROR);
        PrintOperationResult (stderr, OPERATION_PROCESS_ERROR, operationName);

        return STACK_POINTER_NULL;
    }

    if (requestMemory->dataHash == stack->dataHash){
        WriteCommandResponse (OPERATION_SUCCESS);
        PrintOperationResult (stderr, OPERATION_SUCCESS, operationName);
    }else{
        WriteCommandResponse (OPERATION_FAILED);
        PrintOperationResult (stderr, OPERATION_FAILED, operationName);
    }

    return NO_ERRORS;
}

Stack *GetStackFromDescriptor (int stackDescriptor){
    return (Stack *) StackBackups + stackDescriptor;
}

void WriteResult (StackErrorCode errorCode) {

    if (errorCode == NO_ERRORS){
        WriteCommandResponse (OPERATION_SUCCESS);
    }else {
        WriteCommandResponse (OPERATION_FAILED);
    }
}

void WriteCommandResponse (StackCommandResponse response) {
    requestMemory->response = response;
}

#ifndef _NDEBUG

static void PrintSecurityProcessInfo (FILE *stream, MessageType messageType, char *message, ...) {
    custom_assert_without_logger (stream,  pointer_is_null, (void)0);
    custom_assert_without_logger (message, pointer_is_null, (void)0);

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
}

static void PrintOperationResult (FILE *stream, StackCommandResponse response, char *operationName) {
    custom_assert_without_logger (stream,        pointer_is_null, (void)0);
    custom_assert_without_logger (operationName, pointer_is_null, (void)0);

    char *formatString = NULL;
    MessageType type = INFO_MESSAGE;

    switch (response){
        case OPERATION_FAILED:
            formatString = "Stack corruption has been detected (\033[1;36m%s\033[0;37m)\n";
            type = WARNING_MESSAGE;
            break;

            case OPERATION_PROCESSING:
                formatString = "Operation is still processing now (\033[1;36m%s\033[0;37m)\n";
                type = INFO_MESSAGE;
                break;

            case OPERATION_SUCCESS:
                formatString = "Command executed (\033[1;36m%s\033[0;37m)\n";
                type = SUCCESS_MESSAGE;
                break;

            case OPERATION_PROCESS_ERROR:
                formatString = "Error occuried (\033[1;36m%s\033[0;37m)\n";
                type = ERROR_MESSAGE;
                break;

            default:
                return;
                break;
        };

    PrintSecurityProcessInfo (stream, type, formatString, operationName);
}
#else

static void PrintSecurityProcessInfo (FILE *stream, MessageType messageType, char *message, ...) {}
static void PrintOperationResult (FILE *stream, StackCommandResponse response, char *operationName) {}

#endif

//=====================================================================================================================================================================================
//====================================================================PARENT PROCESS PART==============================================================================================
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

    custom_assert (stack, pointer_is_null, STACK_POINTER_NULL);

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

    custom_assert (stack, pointer_is_null, STACK_POINTER_NULL);

    int descriptor = -1;

    Stack *realStack = DecryptStackAddress (stack, &descriptor);

    StackErrorCode errorCode = StackDestruct (realStack);
    errorCode = (StackErrorCode) (ResponseToError (CallBackupOperation (descriptor, STACK_DESTRUCT_COMMAND, 0, realStack)) | errorCode);

    RETURN errorCode;
}


StackErrorCode StackPopSecure (Stack *stack, elem_t *returnValue, const StackCallData callData) {
    PushLog (2);

    custom_assert (stack, pointer_is_null, STACK_POINTER_NULL);

    int descriptor = -1;

    Stack *realStack = DecryptStackAddress (stack, &descriptor);

    WriteError (realStack, CallBackupOperation (descriptor, STACK_VERIFY_COMMAND, 0, realStack));

    StackPop (realStack, returnValue, callData);
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

Stack *DecryptStackAddress (Stack *stack, int *descriptor) {
    // Why are you doing that, dude?

    PushLog (4);

    custom_assert(stack,      pointer_is_null, NULL);
    custom_assert(descriptor, pointer_is_null, NULL);

    char *stackChar = (char *) stack;
    Stack *returnPointer = 0;
    char * returnPointerPointer = (char *) (&returnPointer);
    size_t byteNumber = 0;

    int checksum = 0;

    // Seriously, you have to stop

    for (size_t byteIndex = 0; byteIndex < 4; byteIndex++){
        for (size_t bit = 0; bit < 8; bit++){
            if (stackChar [byteIndex] & (1 << bit)){

                if (byteIndex == 0){
                    RETURN NULL;
                }

                if (byteNumber >= 8){
                    RETURN NULL;
                }

                checksum += stackChar [byteIndex * 8 + bit];

                returnPointerPointer [byteNumber++] = stackChar [byteIndex * 8 + bit];
            }
        }
    }

    if (checksum != *((int *) (void *) (stackChar + 4))){
        RETURN NULL;
    }

    *descriptor = *((int *) (void *) (stackChar + 24));

    // It would be unfair if u copy that :(

    RETURN returnPointer;
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

    for (size_t pointerByte = 0; pointerByte < 8; pointerByte++){
        size_t pointerPosition = 0, addressByte = 0, addressBit = 0;
        do {
            pointerPosition = (size_t) rand () % 2 + pointerByte * 2 + 8;

            addressByte = pointerPosition / 8;
            addressBit  = pointerPosition % 8;
        } while (outPointerChar [addressByte] & 1 << addressBit);

        outPointerChar [pointerPosition] = stackChar [pointerByte];

        checksum += stackChar [pointerByte];

        outPointerChar [addressByte] |= (char) (1 << addressBit);
    }

    *((int *) (void *) (outPointerChar + 4)) = checksum;

    *((int *) (void *) (outPointerChar + 24)) = descriptor;

    RETURN;
}



