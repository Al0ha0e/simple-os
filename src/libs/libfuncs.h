#ifndef LIBFUNCS_H
#define LIBFUNCS_H
#include "types.h"

void *memset(void *dst, int c, size_t n);
void *memcpy(void *dst, const void *src, size_t n);

void printf(char *fmt, ...);

void panic(char *s);

#endif