#define _POSIX_C_SOURCE 200809L
#include "Config.h"
#include "Log.h"
#include <stdlib.h>
#include <string.h>

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
        if (value[0] == 'f' || value[0] == '0') {
            conf->simple = false;
        } else {
            conf->simple = true;
        }
    } else if (!strcmp(key, "linespacing")) {
        conf->linespacing = strtol(value, NULL, 10);
    } else if (!strcmp(key, "progressbarheight")) {
        conf->progress_bar_height = strtol(value, NULL, 10);
    }
    Debug("ConfigKey: {str}  ConfigValue: {str}\n", key, value);
}

