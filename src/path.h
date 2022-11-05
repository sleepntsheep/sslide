#pragma once
#include <stdbool.h>
#include <stddef.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

bool path_is_relative(char *path);
int path_home_dir(char *out, size_t out_size);

