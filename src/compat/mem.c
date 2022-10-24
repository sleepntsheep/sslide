#include "mem.h"
#include <string.h>
#include <stdlib.h>

#ifndef memrchr
void *memrchr(const void *s, int c, size_t n)
{
    const unsigned char *p = (unsigned char*)s + n - 1;
    for (; p >= (unsigned char *)s; p--) 
    {
        if (*p == c) return (void*)p;
    }
    return NULL;
}
#endif

#ifndef strdup
char *strdup(const char *s)
{
    size_t len = 0;
    while (s[len]) len++;
    char *dup = (char*) malloc(len + 1);
    memcpy(dup, s, len);
    dup[len] = 0;
    return dup;
}
#endif

