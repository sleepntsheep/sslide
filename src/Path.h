#pragma once
#include <stdbool.h>
#include <stddef.h>
#include "String.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

bool Path_isrelative(char *path);
String Path_gethome(void);

