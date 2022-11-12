#pragma once
#include <stddef.h>
#include "Renderer.h"
#include "Frame.h"

struct Page {
    size_t alloc;
    size_t length;
    struct Frame *frames;
};

void Page_init(struct Page *page);
void Page_push(struct Page *page, struct Frame frame);
void Page_cleanup(struct Page *page);
void Page_draw(struct Page *page, struct Renderer *r);

