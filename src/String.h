#pragma once
#ifndef STRING_H
#define STRING_H

#include <stddef.h>

typedef char *String;

String String_make(const char *cstr);
String String_format(const char *fmt, ...);
String String_cat(String string, const char *cstr);
String String_catlen(String string, const char *cstr, size_t len);
String String_realloc(String string, size_t newalloc);
size_t String_length(String string);
void String_free(String string);

#endif /* STRING_H */

