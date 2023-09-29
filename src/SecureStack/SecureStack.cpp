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
#include "Logger.h"

typedef StackErrorCode CallbackFunction_t ();

static const unsigned int CycleSleepTime = 1;

static int SecurityProcessPid = -1;
static StackOperationRequest *requestMemory = NULL;

static size_t StackCount = 0;
static void *StackBackups = NULL;

//-----------------------CHILD PROCESS PROTOTYPES----------------

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

//------------------PARENT PROCESS PROTOTYPES--------------------

static StackCommandResponse CallBackupOperation (int descriptor, StackCommand operation, elem_t argument, Stack *stack);
StackErrorCode ResponseToError (StackCommandResponse response);

static void EncryptStackAddress (Stack *stack, Stack *outPointer, int descriptor);
static Stack *DecryptStackAddress (Stack *stack, int *descriptor);

//-------------Child process part----------------------

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
        fprintf_color (CONSOLE_RED, CONSOLE_NORMAL, stderr, "Error occuried while forking\n");

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

        fprintf (stderr, "Current process PID: %d\nSecurity process PID: %d\n", getpid (), SecurityProcessPid);

        return NO_ERRORS;
    }

}


StackErrorCode SecurityProcessDestruct () {

    fprintf (stderr, "Destroying security process...\n");

    for (size_t stackIndex = 0; stackIndex < StackCount; stackIndex++){
        StackDestruct (GetStackFromDescriptor ((int) stackIndex));
    }

    free (StackBackups);

    WriteCommandResponse (OPERATION_SUCCESS);

    if (munmap (requestMemory, sizeof (StackOperationRequest)) != 0){
        fprintf (stderr, "Memory deallocation error\n");
    }

    fprintf (stderr, "Aborting security process...\n");

    exit (0);
    return NO_ERRORS;
}


StackErrorCode SecurityProcessBackupLoop (){

    #define COMMAND_(targetCommand, callback)                       \
        if (targetCommand == requestMemory->command){               \
            callbackFunction = callback;                            \
            fprintf (stderr, "Command %s found\n", #targetCommand); \
        }


    do{
        if (requestMemory->response != OPERATION_PROCESSING)
            continue;


        fprintf (stderr, "Something is received: %d\n", requestMemory->command);

        CallbackFunction_t *callbackFunction = NULL;

        COMMAND_ (STACK_PUSH_COMMAND,     StackPushSecureProcess     );
        COMMAND_ (STACK_POP_COMMAND,      StackPopSecureProcess      );
        COMMAND_ (STACK_INIT_COMMAND,     StackInitSecureProcess     );
        COMMAND_ (STACK_DESTRUCT_COMMAND, StackDestructSecureProcess );
        COMMAND_ (STACK_VERIFY_COMMAND,   VerifyStackSecureProcess   );
        COMMAND_ (ABORT_PROCESS,          SecurityProcessDestruct    );

        if (requestMemory->command == UNKNOWN_COMMAND){
            fprintf (stderr, "something gone wrong\n");
            WriteCommandResponse (OPERATION_FAILED);
        }

        StackErrorCode errorCode = NO_ERRORS;

        if (callbackFunction == NULL)
            continue;

        if ((errorCode = callbackFunction ()) != NO_ERRORS){
            return errorCode;
        }else {
            fprintf (stderr, "Command %d executed\n", requestMemory->command);
            callbackFunction = NULL;
        }

        usleep (CycleSleepTime);

    }while (requestMemory->command != ABORT_PROCESS);

    exit (0);

    #undef COMMAND_

    return NO_ERRORS;
}

StackErrorCode StackInitSecureProcess () {
    fprintf (stderr, "Command executed (secure init)\n");

    if (StackCount >= MAX_STACKS_COUNT){
        WriteCommandResponse (OPERATION_FAILED);

        return SECURITY_PROCESS_ERROR;
    }

    StackErrorCode errorCode = StackInit ((Stack *) StackBackups + StackCount, {"", "", 0, "", false});

    if (errorCode == NO_ERRORS) {
        WriteCommandResponse (OPERATION_SUCCESS);

        requestMemory->stackDescriptor = (int) StackCount;

        StackCount++;
    }else {
        WriteCommandResponse (OPERATION_FAILED);
    }

    return errorCode;
}


StackErrorCode StackDestructSecureProcess () {
    fprintf (stderr, "Command executed (secure destruct)\n");

    StackErrorCode errorCode = StackDestruct (GetStackFromDescriptor (requestMemory->stackDescriptor));

    WriteResult (errorCode);

    return errorCode;
}


StackErrorCode StackPopSecureProcess () {
    fprintf (stderr, "Command executed (secure pop)\n");

    StackErrorCode errorCode =  StackPop (GetStackFromDescriptor (requestMemory->stackDescriptor), NULL, {"","", 0, "", false});

    WriteResult (errorCode);

    return errorCode;
}


StackErrorCode StackPushSecureProcess () {
    fprintf (stderr, "Command executed (secure push)\n");

    StackErrorCode errorCode = StackPush (GetStackFromDescriptor (requestMemory->stackDescriptor), requestMemory->argument, {"","", 0, "", false});

    WriteResult (errorCode);

    return errorCode;
}

StackErrorCode VerifyStackSecureProcess (){
    Stack *stack = GetStackFromDescriptor (requestMemory->stackDescriptor);

    if (stack == NULL)
        return STACK_POINTER_NULL;

    if (requestMemory->dataHash == stack->dataHash && requestMemory->stackHash == stack->stackHash)
        WriteCommandResponse (OPERATION_SUCCESS);

    WriteCommandResponse (OPERATION_FAILED);

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

//-------------Parent process part----------------------

#define WriteError(errorStack, function) (errorStack)->errorCode = (StackErrorCode) (ResponseToError (function) | (errorStack)->errorCode)

StackErrorCode SecurityProcessStop () {
    PushLog (2);

    CallBackupOperation (-1, ABORT_PROCESS, 0, NULL);

    fprintf (stderr, "Waiting for security process %d to stop\n", SecurityProcessPid);

    wait (&SecurityProcessPid);

    //fprintf (stderr, "Wait passed\n");
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

    WriteError ((Stack *)StackBackups + StackCount, CallBackupOperation(descriptor, STACK_VERIFY_COMMAND, 0, (Stack *)StackBackups + StackCount));

    EncryptStackAddress((Stack *)StackBackups + StackCount, stack, descriptor);

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

    WriteError (realStack, CallBackupOperation(descriptor, STACK_VERIFY_COMMAND, 0, realStack));

    StackPop (realStack, returnValue, callData);
    WriteError (realStack, CallBackupOperation (descriptor, STACK_POP_COMMAND,   0, realStack));

    WriteError (realStack, CallBackupOperation(descriptor, STACK_VERIFY_COMMAND, 0, realStack));

    RETURN realStack->errorCode;
}


StackErrorCode StackPushSecure (Stack *stack, elem_t value, const StackCallData callData) {
    PushLog (2);

    custom_assert (stack, pointer_is_null, STACK_POINTER_NULL);

    int descriptor = -1;

    Stack *realStack = DecryptStackAddress (stack, &descriptor);

    WriteError (realStack, CallBackupOperation(descriptor, STACK_VERIFY_COMMAND, 0, realStack));

    StackPush (realStack, value, callData);
    WriteError (realStack, CallBackupOperation (descriptor, STACK_PUSH_COMMAND,  0, realStack));

    WriteError (realStack, CallBackupOperation(descriptor, STACK_VERIFY_COMMAND, 0, realStack));

    RETURN realStack->errorCode;
}

StackCommandResponse CallBackupOperation (int descriptor, StackCommand operation, elem_t argument, Stack *stack){
    PushLog (3);

    requestMemory->stackDescriptor = descriptor;
    requestMemory->argument = argument;
    requestMemory->command = operation;

    if (stack != NULL){
        requestMemory->stackHash = stack->stackHash;
        requestMemory->stackHash = stack->dataHash;
    }

    requestMemory->response = OPERATION_PROCESSING;

    while (requestMemory->response == OPERATION_PROCESSING && operation != ABORT_PROCESS);

    RETURN requestMemory->response;
}

StackErrorCode ResponseToError (StackCommandResponse response){
    return (response == OPERATION_SUCCESS ? NO_ERRORS : EXTERNAL_VERIFY_FAILED);
}


Stack *DecryptStackAddress (Stack *stack, int *descriptor) {
    PushLog (4);

    custom_assert(stack,      pointer_is_null, NULL);
    custom_assert(descriptor, pointer_is_null, NULL);

    char *stackChar = (char *) stack;
    Stack *returnPointer = 0;
    char * returnPointerPointer = (char *) (&returnPointer);
    size_t byteNumber = 0;

    int checksum = 0;

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

    // TODO: symmetric encryption/decription for address with some define shit

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



