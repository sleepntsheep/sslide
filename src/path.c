#include "path.h"
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
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

int path_home_dir(char *out, size_t out_size) {
    char *p = NULL;
#ifdef _WIN32
    p = getenv("USERPROFILE");
    if (ret != NULL) {
        if (strlen(ret) + 1 > out_size) {
            return 1;
        }
        snprintf(out, out_size, "%s", ret);
    } else {
        char *p1 = getenv("HOMEDRIVE");
        char *p2 = getenv("HOMEPATH");
        if (!p1 || !p2 || strlen(p1) + strlen(p2) + 1 > out_size) {
            return 1;
        }
        snprintf(out, out_size, "%s/%s", p1, p2);
    }
#else
    p = getenv("HOME");
    if (!p) {
        p = getpwuid(getuid())->pw_dir;
    }
    if (!p || strlen(p) + 1 > out_size) {
        return 1;
    }
    snprintf(out, out_size, "%s", p);
#endif
    return 0;
}

