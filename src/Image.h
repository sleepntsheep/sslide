#pragma once
#include "Renderer.h"
#include "Rect.h"
#include <stdbool.h>

struct Image {
    SDL_Texture *texture;
    char path[4096];
    float ratio;
    bool valid;
};

void Image_init(struct Image *, char *);
void Image_load_texture(struct Image *, struct Renderer *r);
void Image_draw(struct Image *image, struct Renderer *r, const struct Rect *rect);
void Image_cleanup(struct Image *);

