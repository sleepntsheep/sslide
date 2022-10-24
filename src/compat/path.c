#include "path.h"
#include "mem.h"
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
/* needed for tinyfiledialog and PathIsRelative */
#include <shlwapi.h>
#include <windows.h>
#else
/* needed for getting home directory (getpwuid)
   in case $HOME isn't set */
#include <pwd.h>
#include <unistd.h>
#endif

bool path_is_relative(char *path) {
#ifndef _WIN32 /* path beginning with root is absolute path */
    return path[0] != '/';
#else /* relative path in windows is complicated */
    return PathIsRelative(path);
#endif
}

int path_dirname(const char *path, int path_size, char *result, int result_size) {
    /* if error, return -1
     * if result_size is too low, return how many more byte is needed
     * on success, return 0 */
    ptrdiff_t at = (char*)memrchr(path, '/', path_size) - path;
#ifdef _WIN32
    if (at < 0) at = (char*)memrchr(path, '\\', path_size) - path;
#endif
    if (at < 0) return -1;
    if (at + 1 > result_size) return at + 1 - result_size;
    memcpy(result, path, at);
    result[at] = '\x0';
    return 0;
}

int path_basename(const char *path, int path_size, char *result, int result_size) {
    (void)path;
    (void)path_size;
    (void)result;
    (void)result_size;
    /* TODO: UNIMPLEMENTED */
    return -1;
}

char *path_home_dir(void) {
    char *ret = NULL;
#ifdef _WIN32
    ret = getenv("USERPROFILE");
    if (ret == NULL) {
        ret = calloc(1, PATH_MAX);
        if (ret != NULL) {
            strcat(ret, getenv("HOMEDRIVE"));
            strcat(ret, getenv("HOMEPATH"));
        }
    }
#else
    ret = getenv("HOME");
    if (ret == NULL) {
        ret = getpwuid(getuid())->pw_dir;
    }
#endif
    return ret;
}

