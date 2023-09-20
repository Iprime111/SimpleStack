#include <stdio.h>

#include "CustomAssert.h"
#include "Stack.h"

int main (){
    PushLog (1);

    Stack test_stack = {};

    StackInit (&test_stack);



    double test_variable = 1;

    for (int i = 0; i < 20; i++){
        StackPush (&test_stack, test_variable);
    }
    
    test_stack.capacity = -1;

    StackPop (&test_stack, &test_variable);


    StackDestruct (&test_stack);

    RETURN 0;
}
