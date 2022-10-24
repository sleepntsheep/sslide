#pragma once

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#include <stdbool.h>

bool path_is_relative(char *path);
int path_dirname(const char *path, int path_size, char *result, int result_size);
int path_basename(const char *path, int path_size, char *result, int result_size);
char *path_home_dir(void);

