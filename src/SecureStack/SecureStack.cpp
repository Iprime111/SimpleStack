#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "ColorConsole.h"
#include "CustomAssert.h"
#include "SecureStack/SecureStack.h"
#include "Logger.h"

typedef StackErrorCode CallbackFunction_t (const char *command);

const int SleepTime = 1000;

static int SecurityProcessPid = -1;
static int CommandPipe  [2] = {-1, -1};
static int ResponsePipe [2] = {-1, -1};

static size_t StackCount = 0;
static void *StackBackups = NULL;

//-----------------------CHILD PROCESS PROTOTYPES----------------

static StackErrorCode SecurityProcessDestruct (const char *command);

template <typename elem_t>
static StackErrorCode SecurityProcessBackupLoop ();
static StackCommand ExtractCommand (const char *buffer);

template <typename elem_t>
static StackErrorCode StackPopSecureProcess      (const char *command);
template <typename elem_t>
static StackErrorCode StackPushSecureProcess     (const char *command);
template <typename elem_t>
static StackErrorCode StackInitSecureProcess     (const char *command);
template <typename elem_t>
static StackErrorCode StackDestructSecureProcess (const char *command);
template <typename elem_t>
static StackErrorCode VerifyStackSecureProcess   (const char *command);

template <typename elem_t>
static Stack <elem_t> *GetStackFromDescriptor (int stackDescriptor);
template <typename elem_t>
static Stack <elem_t> *GetStackFromCommand    (const char *command);
static size_t GetStackDescriptor     (const char *command, int *descriptor);

static void WriteCommandResponse (StackCommandResponse response);
static void WriteResult (StackErrorCode errorCode);

//------------------PARENT PROCESS PROTOTYPES--------------------

static StackCommandResponse CallBackupOperation (int descriptor, StackCommand operation, void *arguments, size_t argumentsSize);
StackErrorCode ResponseToError (StackCommandResponse response);
template <typename elem_t>
static void EncryptStackAddress (Stack <elem_t> *stack, Stack <elem_t> *outPointer, int descriptor);
template <typename elem_t>
static Stack <elem_t> *DecryptStackAddress (Stack <elem_t> *stack, int *descriptor);

//-------------Child process part----------------------
template <typename elem_t>
StackErrorCode SecurityProcessInit (int *){

    if (pipe (CommandPipe) < 0){
        fprintf_color (CONSOLE_RED, CONSOLE_NORMAL, stderr, "Error occuried while creating pipe\n");

        return SECURITY_PROCESS_ERROR;
    }

    if (pipe (ResponsePipe) < 0){
        fprintf_color (CONSOLE_RED, CONSOLE_NORMAL, stderr, "Error occuried while creating pipe\n");

        return SECURITY_PROCESS_ERROR;
    }

    SecurityProcessPid = fork ();

    if (SecurityProcessPid < 0){
        fprintf_color (CONSOLE_RED, CONSOLE_NORMAL, stderr, "Error occuried while forking\n");

        return SECURITY_PROCESS_ERROR;
    }else if (SecurityProcessPid == 0){

        if (close (ResponsePipe [0])){
            fprintf_color (CONSOLE_RED, CONSOLE_NORMAL, stderr, "Error occuried while closing response pipe\n");

            return SECURITY_PROCESS_ERROR;
        }

        if (close (CommandPipe [1])){
            fprintf_color (CONSOLE_RED, CONSOLE_NORMAL, stderr, "Error occuried while closing command pipe\n");

            return SECURITY_PROCESS_ERROR;
        }

        StackBackups = calloc (MAX_STACKS_COUNT, sizeof (Stack <elem_t>));

        if (!StackBackups){
            return SECURITY_PROCESS_ERROR;
        }

        SecurityProcessBackupLoop <elem_t> ();

        return NO_ERRORS;
    }else{

        if (close (ResponsePipe [1])){
            fprintf_color (CONSOLE_RED, CONSOLE_NORMAL, stderr, "Error occuried while closing response pipe\n");

            return SECURITY_PROCESS_ERROR;
        }

        if (close (CommandPipe [0])){
            fprintf_color (CONSOLE_RED, CONSOLE_NORMAL, stderr, "Error occuried while closing command pipe\n");

            return SECURITY_PROCESS_ERROR;
        }

        StackBackups = calloc (MAX_STACKS_COUNT, sizeof (Stack <elem_t>));

        fprintf (stderr, "Current process PID: %d\nSecurity process PID: %d\n", getpid (), SecurityProcessPid);

        return NO_ERRORS;
    }

}

template <typename elem_t>
StackErrorCode SecurityProcessDestruct (const char *command) {

    fprintf (stderr, "Destroying security process...\n");

    for (size_t stackIndex = 0; stackIndex < StackCount; stackIndex++){
        StackDestruct (GetStackFromDescriptor <elem_t> ((int) stackIndex));
    }

    free (StackBackups);

    WriteCommandResponse (OPERATION_SUCCESS);

    if (close (ResponsePipe [1])){
        fprintf_color (CONSOLE_RED, CONSOLE_NORMAL, stderr, "Error occuried while closing response pipe\n");

        return SECURITY_PROCESS_ERROR;
    }

    if (close (CommandPipe [0])){
        fprintf_color (CONSOLE_RED, CONSOLE_NORMAL, stderr, "Error occuried while closing command pipe\n");

        return SECURITY_PROCESS_ERROR;
    }

    fprintf (stderr, "Aborting security process...\n");
    exit (0);
    return NO_ERRORS;
}

template <typename elem_t>
StackErrorCode SecurityProcessBackupLoop (){
    char commandBuffer [PIPE_BUFFER_SIZE] = "";

    #define COMMAND_(targetCommand, callback)   \
        if (targetCommand == command){          \
            callbackFunction = callback;        \
        }

    StackCommand command = UNKNOWN_COMMAND;

    do{
        if (read (CommandPipe [0], commandBuffer, sizeof (commandBuffer)) == 0)
            continue;

        command = ExtractCommand (commandBuffer);

        CallbackFunction_t *callbackFunction = NULL;

        COMMAND_ (STACK_PUSH_COMMAND,     StackPushSecureProcess     <elem_t>);
        COMMAND_ (STACK_POP_COMMAND,      StackPopSecureProcess      <elem_t>);
        COMMAND_ (STACK_INIT_COMMAND,     StackInitSecureProcess     <elem_t>);
        COMMAND_ (STACK_DESTRUCT_COMMAND, StackDestructSecureProcess <elem_t>);
        COMMAND_ (STACK_VERIFY_COMMAND,   VerifyStackSecureProcess   <elem_t>);
        COMMAND_ (ABORT_PROCESS,          SecurityProcessDestruct    <elem_t>);

        StackErrorCode errorCode = NO_ERRORS;

        if (callbackFunction == NULL)
            continue;

        if ((errorCode = callbackFunction (commandBuffer)) != NO_ERRORS){
            return errorCode;
        }else {
            fprintf (stderr, "Command %d executed\n", command);
            callbackFunction = NULL;
        }

    }while (command != ABORT_PROCESS);

    #undef COMMAND_

    return NO_ERRORS;
}

StackCommand ExtractCommand (const char *buffer) {
    StackCommand command = UNKNOWN_COMMAND;
    char *commandPointer = (char *) &command;

    for (size_t byteIndex = 0; byteIndex < sizeof (StackCommand); byteIndex++){
        commandPointer [byteIndex] = buffer [byteIndex];
    }

    return command;
}

template <typename elem_t>
StackErrorCode StackInitSecureProcess (const char *command) {
    fprintf (stderr, "Command executed (secure init)\n");

    if (StackCount >= MAX_STACKS_COUNT){
        WriteCommandResponse (OPERATION_FAILED);

        return SECURITY_PROCESS_ERROR;
    }

    StackErrorCode errorCode = StackInit ((Stack <elem_t> *) StackBackups + StackCount, {"", "", 0, "", false});

    if (errorCode == NO_ERRORS) {
        WriteCommandResponse (OPERATION_SUCCESS);

        write (ResponsePipe [1], (char *) &StackCount, sizeof (size_t));

        StackCount++;
    }else {
        WriteCommandResponse (OPERATION_FAILED);
    }

    return errorCode;
}

template <typename elem_t>
StackErrorCode StackDestructSecureProcess (const char *command) {
    fprintf (stderr, "Command executed (secure destruct)\n");

    StackErrorCode errorCode = StackDestruct (GetStackFromCommand <elem_t> (command));

    WriteResult (errorCode);

    return errorCode;
}

template <typename elem_t>
StackErrorCode StackPopSecureProcess (const char *command) {
    fprintf (stderr, "Command executed (secure pop)\n");

    StackErrorCode errorCode =  StackPop (GetStackFromCommand <elem_t> (command), NULL, {"","", 0, "", false});

    WriteResult (errorCode);

    return errorCode;
}

template <typename elem_t>
StackErrorCode StackPushSecureProcess (const char *command) {
    fprintf (stderr, "Command executed (secure push)\n");

    int stackDescriptor = 0;
    elem_t value{};

    char *valuePointer = (char *) &value;

    size_t descriptorIndex = GetStackDescriptor(command, &stackDescriptor);

    for (size_t elementIndex = 0; elementIndex < sizeof (elem_t); elementIndex++) {
        valuePointer [elementIndex] = command [elementIndex + descriptorIndex];
    }

    Stack <elem_t> *stack = GetStackFromDescriptor <elem_t> (stackDescriptor);

    StackErrorCode errorCode = StackPush (stack, value, {"","", 0, "", false});

    WriteResult (errorCode);

    return errorCode;
}

StackErrorCode VerifyStackSecureProcess (const char *command){
    WriteCommandResponse (OPERATION_SUCCESS);

    return NO_ERRORS;
}

size_t GetStackDescriptor (const char *command, int *descriptor){
    size_t stringIndex = 0;

    while (command [stringIndex] != '\0'){
        stringIndex++;
    }

    stringIndex++;

    char *descriptorPointer = (char *) descriptor;
    size_t descriptorIndex = 0;

    for (descriptorIndex = 0; descriptorIndex < 4; descriptorIndex++){
        descriptorPointer [descriptorIndex] = command [descriptorIndex + stringIndex];
    }

    return stringIndex + descriptorIndex;
}

template <typename elem_t>
Stack <elem_t> *GetStackFromDescriptor (int stackDescriptor){
    return (Stack <elem_t> *) StackBackups + stackDescriptor;
}

template <typename elem_t>
Stack <elem_t> *GetStackFromCommand (const char *command){
    int stackDescriptor = 0;
    GetStackDescriptor(command, &stackDescriptor);

    return GetStackFromDescriptor <elem_t> (stackDescriptor);
}

void WriteResult (StackErrorCode errorCode) {

    if (errorCode == NO_ERRORS){
        WriteCommandResponse (OPERATION_SUCCESS);
    }else {
        WriteCommandResponse (OPERATION_FAILED);
    }
}

void WriteCommandResponse (StackCommandResponse response) {
    write (ResponsePipe [1], &response, sizeof (StackCommandResponse));
}

//-------------Parent process part----------------------

#define WriteError(errorStack, function) (errorStack)->errorCode = (StackErrorCode) (ResponseToError (function) | (errorStack)->errorCode)

StackErrorCode SecurityProcessStop (int securityProcessPID) {
    PushLog (2);

    CallBackupOperation (-1, ABORT_PROCESS, NULL, 0);

    fprintf (stderr, "Waiting for security process to stop\n");

    wait (NULL);

    fprintf (stderr, "Wait passed\n");
    RETURN NO_ERRORS;
}

template <typename elem_t>
StackErrorCode StackInitSecure (elem_t *stack, const StackCallData callData, ssize_t initialCapacity) {
    PushLog (2);

    custom_assert (stack, pointer_is_null, STACK_POINTER_NULL);

    int descriptor = -1;

    StackInit ((Stack <elem_t> *) StackBackups + StackCount, callData, initialCapacity);
    StackCommandResponse response = CallBackupOperation (descriptor, STACK_INIT_COMMAND, NULL, 0);

    if (response == OPERATION_SUCCESS){
        while (read (ResponsePipe [0], &descriptor, sizeof (descriptor)) == 0);
    }else{
        WriteError ((Stack <elem_t> *)StackBackups + StackCount, response);
    }

    WriteError ((Stack <elem_t> *)StackBackups + StackCount, CallBackupOperation(descriptor, STACK_VERIFY_COMMAND, NULL, 0));

    EncryptStackAddress((Stack <elem_t> *)StackBackups + StackCount, stack, descriptor);

    RETURN ((Stack <elem_t> *)StackBackups + StackCount)->errorCode;
}

template <typename elem_t>
StackErrorCode StackDestructSecure (Stack <elem_t> *stack){
    PushLog (2);

    custom_assert (stack, pointer_is_null, STACK_POINTER_NULL);

    int descriptor = -1;

    Stack <elem_t> *realStack = DecryptStackAddress (stack, &descriptor);

    StackErrorCode errorCode = StackDestruct (realStack);
    errorCode = (StackErrorCode) (ResponseToError (CallBackupOperation (descriptor, STACK_DESTRUCT_COMMAND, NULL, 0)) | errorCode);

    RETURN errorCode;
}

template <typename elem_t>
StackErrorCode StackPopSecure (Stack <elem_t> *stack, elem_t *returnValue, const StackCallData callData) {
    PushLog (2);

    custom_assert (stack, pointer_is_null, STACK_POINTER_NULL);

    int descriptor = -1;

    Stack <elem_t> *realStack = DecryptStackAddress (stack, &descriptor);

    WriteError (realStack, CallBackupOperation(descriptor, STACK_VERIFY_COMMAND, NULL, 0));

    StackPop (realStack, returnValue, callData);
    WriteError (realStack, CallBackupOperation (descriptor, STACK_POP_COMMAND, NULL, 0));

    WriteError (realStack, CallBackupOperation(descriptor, STACK_VERIFY_COMMAND, NULL, 0));

    RETURN realStack->errorCode;
}

template <typename elem_t>
StackErrorCode StackPushSecure (Stack <elem_t> *stack, elem_t value, const StackCallData callData) {
    PushLog (2);

    custom_assert (stack, pointer_is_null, STACK_POINTER_NULL);

    int descriptor = -1;

    Stack <elem_t> *realStack = DecryptStackAddress (stack, &descriptor);

    WriteError (realStack, CallBackupOperation(descriptor, STACK_VERIFY_COMMAND, NULL, 0));

    StackPush (realStack, value, callData);
    WriteError (realStack, CallBackupOperation (descriptor, STACK_PUSH_COMMAND, &value, sizeof (elem_t)));

    WriteError (realStack, CallBackupOperation(descriptor, STACK_VERIFY_COMMAND, NULL, 0));

    RETURN realStack->errorCode;
}

StackCommandResponse CallBackupOperation (int descriptor, StackCommand operation, void *arguments, size_t argumentsSize){
    PushLog (3);

    write (CommandPipe [1], &operation,  sizeof (StackCommand));

    write (CommandPipe [1], &descriptor, sizeof (int));

    if (arguments || argumentsSize == 0){
        write (CommandPipe [1], arguments, argumentsSize);
    }

    StackCommandResponse response = OPERATION_FAILED;

    while (read (ResponsePipe [0], &response, sizeof (StackCommand)) == 0);

    RETURN response;
}

StackErrorCode ResponseToError (StackCommandResponse response){
    return (response == OPERATION_SUCCESS ? NO_ERRORS : EXTERNAL_VERIFY_FAILED);
}

template <typename elem_t>
Stack <elem_t> *DecryptStackAddress (Stack <elem_t> *stack, int *descriptor) {
    PushLog (4);

    custom_assert(stack,      pointer_is_null, NULL);
    custom_assert(descriptor, pointer_is_null, NULL);

    char *stackChar = (char *) stack;
    Stack <elem_t> *returnPointer = 0;
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

template <typename elem_t>
void EncryptStackAddress (Stack <elem_t> *stack, Stack <elem_t> *outPointer, int descriptor) {
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



