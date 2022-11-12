#include "Path.h"
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
#include "String.h"

bool Path_isrelative(const char *path) {
    if (!path) return false;
#ifndef _WIN32 /* path beginning with root is absolute path */
    return path[0] != '/';
#else /* relative path in windows is complicated */
    return PathIsRelative(path);
#endif
}

String Path_gethome(void) {
#ifdef _WIN32
    char *p = getenv("USERPROFILE");
    if (p) return String_make(p);
    return String_format("%s/%s", getenv("HOMEDRIVE"), getenv("HOMEPATH"));
#else
    char *p = getenv("HOME");
    if (!p)
        p = getpwuid(getuid())->pw_dir;
    return String_make(p);
#endif
    return NULL;
}

String Path_dirname(const char *path) {
    if (!path) return NULL;
#ifdef _WIN32
    /* note - im not sure if this WIN32 version work */
    String s = String_make(path);
    char *p = strrchr(s, '/');
    if (p) {
        *p = 0;
    } else {
        p = strrchr(s, '\\');
        *p = 0;
    }
    return s;
#else
    String s = String_make(path);
    char *p = strrchr(s, '/');
    if (p) *p = 0;
    return s;
#endif
}

