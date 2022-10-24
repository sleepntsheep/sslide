#pragma once

#include <stddef.h>

#ifndef memrchr
void *memrchr(const void *s, int c, size_t n);
#endif

#ifndef strdup
char *strdup(const char *s);
#endif
