#include "Config.h"
#include "Log.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static long strtol_wrap(const char *s) {
    errno = 0;
    long r = strtol(s, NULL, 10);
    if (errno != 0) {
        Warn("Failed converting %s to integer", s);
    }
    return r;
}

void config_parse_line(struct Config *conf, char *line)
{
    if (line == NULL) return;
    char *sep = strchr(line, '=');
    if (sep == NULL) return;
    *sep = 0;
    char *key = line;
    char *value = sep + 1;
    while (*value == ' ') value++;
    if (!strcmp(key, "fg") || !strcmp(key, "bg")) {
        uint32_t tmp = strtol(value, NULL, 16);
        SDL_Color *c = *key == 'f' ? &conf->fg : &conf->bg;
        c->r = (uint8_t)((tmp & 0xFF0000) >> 16);
        c->g = (uint8_t)((tmp & 0x00FF00) >> 8);
        c->b = (uint8_t)(tmp & 0x0000FF);
        c->a = 0xFF;
    } else if (!strcmp(key, "font")) {
        conf->font = String_make(value);
    } else if (!strcmp(key, "simple")) {
        conf->simple = !strcmp(value, "true");
    } else if (!strcmp(key, "linespacing")) {
        conf->linespacing = strtol(value, NULL, 10);
    } else if (!strcmp(key, "progressbarheight")) {
        conf->progress_bar_height = strtol(value, NULL, 10);
    } else if (!strcmp(key, "marginx")) {
        conf->margin_x = strtol_wrap(value);
    } else if (!strcmp(key, "marginy")) {
        conf->margin_y = strtol_wrap(value);
    }
    Debug("ConfigKey: {str}  ConfigValue: {str}\n", key, value);
}

