#ifndef PROC_PARSER_H_
#define PROC_PARSER_H_

#include <cstddef>

const size_t MaxLineSize = 4096;

struct DataSegment {
    char *begin;
    char *end;
};

bool IsAddressValid (const void *address);

#endif
