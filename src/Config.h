#pragma once
#ifndef SSLIDE_CONFIG_H
#define SSLIDE_CONFIG_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include "String.h"

struct Config {
    String font;
    SDL_Color bg;
    SDL_Color fg;
    bool simple;
    int progress_bar_height;
    int linespacing;
    int margin_x;
    int margin_y;
};

static const struct Config config_default = {
    .font = NULL,
    .bg = {0xFF, 0xFF, 0xFF, 0xFF},
    .fg = {0, 0, 0, 0xFF},
    .simple = false,
    .progress_bar_height = 5,
    .linespacing = 3,
    .margin_x = 10,
    .margin_y = 10,
};

void config_parse_line(struct Config *conf, char *line);

#endif /* SSLIDE_CONFIG_H */

