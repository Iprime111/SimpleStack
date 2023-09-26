#include <stdio.h>
#include <sys/wait.h>

#include "CustomAssert.h"
#include "Stack.h"
#include "SecureStack.h"

//TODO use alingment
int main() {
    PushLog (1);

    SecurityProcessInit ();

    Stack test_stack = {};

    StackInitDefaultSecure_ (&test_stack);

    double test_variable = 1;

    // TODO unit tets on Gtests
    for (int i = 0; i < 15; i++){
        StackPushSecure_ (&test_stack, test_variable);
    }

    //test_stack.stackHash = 0;

     printf ("Popping value from stack...\n");

    StackPopSecure_ (&test_stack, &test_variable);

    printf ("Destroying stack...\n");

    StackDestructSecure_ (&test_stack);

    printf ("Stopping security process...\n");

    SecurityProcessStop ();

    RETURN 0;
}
