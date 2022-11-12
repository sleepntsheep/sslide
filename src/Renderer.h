#pragma once
#ifndef RENDERER_H
#define RENDERER_H
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>

struct Renderer {
    SDL_Renderer *rend;
    SDL_Window *win;
    int width;
    int height;
};

void Renderer_global_init(void);
void Renderer_global_cleanup(void);

void Renderer_init(struct Renderer *r, const char *title);
void Renderer_cleanup(struct Renderer *r);

#endif /* RENDERER_H */

