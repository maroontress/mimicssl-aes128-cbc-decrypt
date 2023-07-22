#ifndef libext1_H
#define libext1_H

#include <string.h>

#if defined(__STDC_LIB_EXT1__) || defined(_WIN32)
#include <stdlib.h>
static void *
MEMCPY(void *x, const void *y, size_t z)
{
    if (memcpy_s(x, z, y, z) != 0) {
        abort();
    }
    return x;
}
#else
#define MEMCPY(x, y, z) memcpy(x, y, z)
#endif

#endif
