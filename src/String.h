#pragma once
#ifndef STRING_H
#define STRING_H

#include <stddef.h>

typedef char *String;
typedef String *StringArray;

String String_make(const char *cstr);
String String_format(const char *fmt, ...);
String String_cat(String string, const char *cstr);
String String_catlen(String string, const char *cstr, size_t len);
String String_realloc(String string, size_t newalloc);
size_t String_length(String string);
void String_free(String string);

StringArray StringArray_new(void);
size_t StringArray_alloc(StringArray sa);
size_t StringArray_length(StringArray sa);
StringArray StringArray_realloc(StringArray sa, size_t newalloc);
StringArray StringArray_push_String(StringArray sa, String s);
StringArray StringArray_push(StringArray sa, const char *cstr);
void StringArray_free(StringArray sa);
String StringArray_join(StringArray sa, const char *delim);

#endif /* STRING_H */

