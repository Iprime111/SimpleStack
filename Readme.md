# Simple stack

## Overview

This is a simple stack data structure with a some data protection methods implemented : )

There are a couple of vulnerabilities, so you can easily hack it. Good luck!

## Building

To build and run this program you need to have `git`, `cmake` and `clang` or `g++` installed on your computer. Simply run this commands:

``` bash
$ git clone https://github.com/Iprime111/SimpleStack.git
$ cd SimpleStack
$ git submodule update --init --remote --recursive
$ mkdir build
$ cd build
$ cmake ..
$ make
$ ./src/simple_stack
```
## Functions
If you want to have a real challenge use only macro functions with `Secure_` postfix.<br /><br />
All functions listed below return `StackErrorCode` code of error that have been occuried or `NO_ERROR` if there was no such a situation.

<br /><br />

### `StackErrorCode SecurityProcessInit ()`
Launches security process needed by `Secure_` functions. Should be called before any other secured stack operations.
<br /><br />

### `StackErrorCode SecurityProcessStop ()`
Stops security process. Should be called at the end of a program.
<br /><br />

### `StackErrorCode StackInitSecure_ (Stack *stack)`
Inits new stack. Parameter is a pointer to an existing `Stack` data structure.
<br /><br />

### `StackErrorCode StackDestructSecure_ (Stack *stack)`
Destructs existing `Stack` data structure making it unusable.
<br /><br />

### `StackErrorCode StackPushSecure (Stack *stack, elem_t value)`
Pushes new `value` to the given `Stack` structure. Takes `value` as an argument.
<br /><br />

### `StackErrorCode StackPopSecure (Stack *stack, elem_t *returnValue)`
Pops value from the given `Stack` structure and writes it into `returnValue` variable.
<br /><br />

## Compilation defines
There are a couple of compilation defines that have an influence on program's behaviour:
### `_NDEBUG`
Turns off debug mode and switches off assert functions and debug output.

### `_SHOW_STACK_TRACE`
Turns on function logging and backtrace print when assert or stack verify are triggered.

### `_USE_CANARY`
Turns on canary stack structure and stack data protection.

### `_USE_HASH`
Turns on hash stack structure and stack data protection

### `_USE_ADDRESS_VALIDATION`
Turns on address validation function, which examines pointers addresses more properly.

## !!!WARNING!!!
Using address validation and `Secure_` stack functions can hardly damage perfomance and memory usage.

