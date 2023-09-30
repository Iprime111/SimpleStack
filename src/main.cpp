#include <stdio.h>
#include <sys/wait.h>

#include "CustomAssert.h"
#include "Stack/Stack.h"
#include "SecureStack/SecureStack.h"

int main() {
    PushLog (1);

    //SecurityProcessInit ();

    Stack test_stack = {};

    StackInitDefault_ (&test_stack);

    elem_t test_variable = 5;

    for (int i = 0; i < 15; i++){
        StackPushSecure_ (&test_stack, test_variable);
    }

    fprintf (stderr, "Popping value from stack...\n");

    StackPopSecure_ (&test_stack, &test_variable);

    fprintf (stderr, "Destroying stack...\n");

    StackDestructSecure_ (&test_stack);

    fprintf (stderr, "Stopping security process...\n");

    //SecurityProcessStop ();

    RETURN 0;
}
