#include "libfuncs.h"

void *memset(void *dst, int c, size_t n)
{
    char *cdst = (char *)dst;
    int i;
    for (i = 0; i < n; i++)
    {
        cdst[i] = c;
    }
    return dst;
}

void *memcpy(void *dst, const void *src, size_t n)
{
    char *d = dst;
    for (int i = 0; i < n; i++)
        d[i] = ((char *)src)[i];
    return dst;
}