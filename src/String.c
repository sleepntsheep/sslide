#include "String.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static size_t first_2n_not_less_than(size_t x) {
    size_t res = 1;
    while (res < x) {
        res *= 2;
    }
    return res;
}

static String String_ensure_alloc(String string, size_t enough_for) {
    size_t newalloc = first_2n_not_less_than(enough_for + String_length(string));
    return String_realloc(string, newalloc);
}

struct string {
    size_t alloc;
    size_t len;
    char buf[];
};

struct string_array {
    size_t alloc;
    size_t len;
    String buf[];
};

static struct string *String_getinfo(String string) {
    return (struct string*)((char*)string - sizeof(struct string));
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
    string = String_ensure_alloc(string, len + 1);
    memcpy(string + String_length(string), cstr, len);
    string[len] = 0;
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

static struct string_array *StringArray_getinfo(StringArray sa) {
    return (struct string_array*)((char*)sa - sizeof(struct string_array));
}

static StringArray StringArray_ensure_alloc(StringArray sa, size_t enough_for) {
    size_t newalloc = first_2n_not_less_than(enough_for + StringArray_length(sa));
    return StringArray_realloc(sa, newalloc);
}

StringArray StringArray_new(void) {
    struct string_array *sa = malloc(sizeof *sa + 4 * sizeof (String));
    if (!sa) return NULL;
    sa->alloc = 4;
    sa->len = 0;
    return sa->buf;
}

size_t StringArray_alloc(StringArray sa) {
    return StringArray_getinfo(sa)->alloc;
}

size_t StringArray_length(StringArray sa) {
    return StringArray_getinfo(sa)->len;
}

StringArray StringArray_realloc(StringArray sa, size_t newalloc) {
    if (!sa) return NULL;
    if (newalloc <= StringArray_alloc(sa)) return sa;
    StringArray_getinfo(sa)->alloc = newalloc;
    struct string_array *newsa = realloc(StringArray_getinfo(sa),
            sizeof(struct string_array) + newalloc * sizeof(String));
    if (!newsa) return NULL;
    return newsa->buf;
}

StringArray StringArray_push_String(StringArray sa, String s) {
    if (!sa) return NULL;
    sa = StringArray_ensure_alloc(sa, 1);
    StringArray_getinfo(sa)->buf[StringArray_length(sa)] = s;
    StringArray_getinfo(sa)->len++;
    return sa;
}

StringArray StringArray_push(StringArray sa, const char *cstr) {
    return StringArray_push_String(sa, String_make(cstr));
}

void StringArray_free(StringArray sa) {
    if (!sa) return;
    for (size_t i = 0; i < StringArray_length(sa); i++)
        String_free(sa[i]);
    free(StringArray_getinfo(sa));
}

String StringArray_join(StringArray sa, const char *delim) {
    if (StringArray_length(sa) > 0) {
        String s = String_make(sa[0]);
        size_t delimlen = strlen(delim);
        for (size_t i = 1; i < StringArray_length(sa); i++) {
            String_catlen(s, delim, delimlen);
            String_catlen(s, sa[i], String_length(sa[i]));
        }
        return s;
    }
    return String_make("");
}

