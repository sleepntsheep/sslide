#include "Renderer.h"
#include "Log.h"

void Renderer_global_init(void) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
        Warn("SDL_Init: %s\n", SDL_GetError());
    if (TTF_Init() < 0)
        Warn("TTF_Init: %s\n", TTF_GetError());
    if (IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_WEBP) < 1)
        Warn("Img_INIT: %s\n", IMG_GetError());
}

void Renderer_global_cleanup(void) {
    SDL_Quit();
    TTF_Quit();
    IMG_Quit();
}

void Renderer_init(struct Renderer *r, const char *title) {
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    r->win = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_RESIZABLE);
    if (!r->win)
        Warn("Failed creating window: %s", SDL_GetError());
    r->rend = SDL_CreateRenderer(r->win, -1, 0);
    if (!r->rend)
        Warn("Failed creating renderer: %s", SDL_GetError());
    Info("Initialized window\n");
}

void Renderer_cleanup(struct Renderer *r) {
    SDL_DestroyRenderer(r->rend);
    SDL_DestroyWindow(r->win);
}

