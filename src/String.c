#include "String.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*
size_t String_first_2n_not_less_than(size_t x) {
    size_t res = 1;
    while (res < x) {
        res *= 2;
    }
    return res;
}

char *String_ensure_alloc(char *string, size_t enough_for) {
    size_t newalloc = String_first_2n_not_less_than(enough_for + String_length(string));
    string = String_realloc(newalloc);
}
*/

struct string {
    size_t alloc;
    size_t len;
    char buf[];
};

struct string *String_getinfo(String string) {
    return (struct string*)(string - sizeof(struct string));
}

size_t String_alloc(String string) {
    return String_getinfo(string)->alloc;
}

size_t String_length(String string) {
    return String_getinfo(string)->len;
}

String String_make(const char *cstr) {
    size_t len = strlen(cstr);
    struct string *string = malloc(sizeof *string + len + 1);
    if (!string) return NULL;
    string->alloc = len + 1;
    string->len = len;
    memcpy(string->buf, cstr, len);
    string->buf[len] = 0;
    return string->buf;
}

String String_realloc(String string, size_t newalloc) {
    if (string == NULL) return NULL;
    if (newalloc < String_alloc(string)) {
        return string;
    }
    String_getinfo(string)->alloc = newalloc;
    struct string *newstring = realloc(String_getinfo(string), sizeof(struct string) + newalloc);
    return newstring->buf;
}

String String_cat(String string, const char *cstr) {
    return String_catlen(string, cstr, strlen(cstr));
}

String String_catlen(String string, const char *cstr, size_t len) {
    if (string == NULL) return NULL;
    size_t alloc = String_length(string) + len + 1;
    string = String_realloc(string, alloc);
    memcpy(string + String_length(string), cstr, len);
    string[alloc-1] = 0;
    String_getinfo(string)->len += len;
    return string;
}

String String_format(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    size_t len = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    struct string *string = malloc(sizeof *string + len + 1);
    string->alloc = len + 1;
    string->len = len;
    va_start(args, fmt);
    vsnprintf(string->buf, len + 1, fmt, args);
    va_end(args);
    return string->buf;
}

void String_free(String string) {
    if (string == NULL) return;
    free(String_getinfo(string));
}


