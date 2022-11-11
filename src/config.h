#pragma once
#ifndef SSLIDE_CONFIG_H
#define SSLIDE_CONFIG_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

typedef struct {
    char *font;
    SDL_Color bg;
    SDL_Color fg;
    bool simple;
    int linespacing;
} Config;

static const Config default_config = {
    .font = NULL,
    .bg = {0xFF, 0xFF, 0xFF, 0xFF},
    .fg = {0, 0, 0, 0xFF},
    .simple = false,
    .linespacing = 3,
};

void config_parse_line(Config *conf, char *line);

#endif /* SSLIDE_CONFIG_H */

