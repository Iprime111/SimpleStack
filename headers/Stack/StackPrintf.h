#ifndef STACK_PRINTF_H_
#define STACK_PRINTF_H_

#include "ColorConsole.h"

template <typename T>
void print_data (enum CONSOLE_COLOR color, enum CONSOLE_BOLD bold, FILE *stream, T data);

template <typename T>
void print_data (enum CONSOLE_COLOR color, enum CONSOLE_BOLD bold, FILE *stream, T data) {
    fprintf_color (color, bold, stream, "%x", data);
}

template <>
inline void print_data (enum CONSOLE_COLOR color, enum CONSOLE_BOLD bold, FILE *stream, int data) {
    fprintf_color (color, bold, stream, "%d", data);
}

template <>
inline void print_data (enum CONSOLE_COLOR color, enum CONSOLE_BOLD bold, FILE *stream, unsigned int data) {
    fprintf_color (color, bold, stream, "%u", data);
}

template <>
inline void print_data (enum CONSOLE_COLOR color, enum CONSOLE_BOLD bold, FILE *stream, long data) {
    fprintf_color (color, bold, stream, "%ld", data);
}

template <>
inline void print_data (enum CONSOLE_COLOR color, enum CONSOLE_BOLD bold, FILE *stream, unsigned long data) {
    fprintf_color (color, bold, stream, "%lu", data);
}

template <>
inline void print_data (enum CONSOLE_COLOR color, enum CONSOLE_BOLD bold, FILE *stream, long long data) {
    fprintf_color (color, bold, stream, "%lld", data);
}

template <>
inline void print_data (enum CONSOLE_COLOR color, enum CONSOLE_BOLD bold, FILE *stream, unsigned long long data) {
    fprintf_color (color, bold, stream, "%llu", data);
}

template <>
inline void print_data (enum CONSOLE_COLOR color, enum CONSOLE_BOLD bold, FILE *stream, double data) {
    fprintf_color (color, bold, stream, "%lf", data);
}

template <>
inline void print_data (enum CONSOLE_COLOR color, enum CONSOLE_BOLD bold, FILE *stream, char *data) {
    fprintf_color (color, bold, stream, data);
}

#endif
