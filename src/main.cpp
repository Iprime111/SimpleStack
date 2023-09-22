#include <stdio.h>

#include "CustomAssert.h"
#include "Stack.h"

int main (){
    PushLog (1);

    Stack test_stack = {};

    StackInitDefault_ (&test_stack);

    double test_variable = 1;

    for (int i = 0; i < 20; i++){
        StackPush_ (&test_stack, test_variable);
    }

    test_stack.capacity = -1;

    StackPop_ (&test_stack, &test_variable);


    StackDestruct_ (&test_stack);

    RETURN 0;
}
