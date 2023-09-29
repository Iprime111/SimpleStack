#include <stdio.h>
#include <malloc.h>

#include "Stack/ProcParser.h"

#ifdef _USE_ADDRESS_VALIDATION

bool IsAddressValid (const void *address) {
    FILE *maps = fopen ("/proc/self/maps", "r");

    if (!maps)
        return false;

    unsigned long minAddress = 0;
    unsigned long maxAddress = 0;
    unsigned long addressInt = (unsigned long) address;

    char dumpBuf [2048] = "";

    while (fscanf(maps, "%lx-%lx", &minAddress, &maxAddress) != EOF) {
        fgets (dumpBuf, 2048, maps);
        if (addressInt <= maxAddress && addressInt >= minAddress)
            return true;
    }

    fclose (maps);


    return false;
}
#else
    bool IsAddressValid (const void *address) {
        return address;
    }
#endif
