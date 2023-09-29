#ifndef STACK_PRINTF_H_
#define STACK_PRINTF_H_

#include "ColorConsole.h"

template <typename T>
void PrintData (enum CONSOLE_COLOR color, enum CONSOLE_BOLD bold, FILE *stream, T data);

template <typename T>
void PrintData (enum CONSOLE_COLOR color, enum CONSOLE_BOLD bold, FILE *stream, T data) {
    fprintf_color (color, bold, stream, "%x", data);
}

template <>
inline void PrintData <char> (enum CONSOLE_COLOR color, enum CONSOLE_BOLD bold, FILE *stream, char data) {
    fprintf_color (color, bold, stream, "%c", data);
}

template <>
inline void PrintData <int> (enum CONSOLE_COLOR color, enum CONSOLE_BOLD bold, FILE *stream, int data) {
    fprintf_color (color, bold, stream, "%d", data);
}

template <>
inline void PrintData <unsigned int> (enum CONSOLE_COLOR color, enum CONSOLE_BOLD bold, FILE *stream, unsigned int data) {
    fprintf_color (color, bold, stream, "%u", data);
}

template <>
inline void PrintData <long> (enum CONSOLE_COLOR color, enum CONSOLE_BOLD bold, FILE *stream, long data) {
    fprintf_color (color, bold, stream, "%ld", data);
}

template <>
inline void PrintData <unsigned long> (enum CONSOLE_COLOR color, enum CONSOLE_BOLD bold, FILE *stream, unsigned long data) {
    fprintf_color (color, bold, stream, "%lu", data);
}

template <>
inline void PrintData <long long> (enum CONSOLE_COLOR color, enum CONSOLE_BOLD bold, FILE *stream, long long data) {
    fprintf_color (color, bold, stream, "%lld", data);
}

template <>
inline void PrintData <unsigned long long> (enum CONSOLE_COLOR color, enum CONSOLE_BOLD bold, FILE *stream, unsigned long long data) {
    fprintf_color (color, bold, stream, "%llu", data);
}

template <>
inline void PrintData <double> (enum CONSOLE_COLOR color, enum CONSOLE_BOLD bold, FILE *stream, double data) {
    fprintf_color (color, bold, stream, "%lf", data);
}

template <>
inline void PrintData <char *> (enum CONSOLE_COLOR color, enum CONSOLE_BOLD bold, FILE *stream, char *data) {
    fprintf_color (color, bold, stream, data);
}

#endif
