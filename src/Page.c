#include "Page.h"
#include "Global.h"

void Page_init(struct Page *page) {
    page->alloc = 4;
    page->length = 0;
    page->frames = malloc(sizeof(struct Frame) * page->alloc);
}

void Page_push(struct Page *page, struct Frame frame) {
    if (page->length == page->alloc) {
        page->frames = realloc(page->frames, sizeof(struct Frame) * (page->alloc *= 2));
    }
    page->frames[page->length++] = frame;
}

void Page_cleanup(struct Page *page) {
    for (size_t i = 0; i < page->length; i++) {
        Frame_cleanup(&page->frames[i]);
    }
    free(page->frames);
}

void Page_draw(struct Page *page, struct Renderer *r) {
    SDL_SetRenderDrawColor(r->rend, config.bg.r, config.bg.g, config.bg.b, config.bg.a);
    SDL_RenderClear(r->rend);
    for (size_t i = 0; i < page->length; i++)
        Frame_draw(page->frames + i, r);
}

