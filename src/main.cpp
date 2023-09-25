#include <stdio.h>

#include "CustomAssert.h"
#include "Stack.h"
#include "SecureStack.h"

//TODO use alingment
int main() {
    PushLog (1);

    Stack test_stack = {};

    StackInitDefault_ (&test_stack);

    double test_variable = 1;

    // TODO unit tets on Gtests
    for (int i = 0; i < 15; i++){
        StackPush_ (&test_stack, test_variable);
    }

    //test_stack.stackHash = 0;

    StackPop_ (&test_stack, &test_variable);


    StackDestruct_ (&test_stack);

    RETURN 0;
}
