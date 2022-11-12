#pragma once
#ifndef SSLIDE_H
#define SSLIDE_H
#include <stddef.h>
#include "Frame.h"
#include "Page.h"

struct Slide {
    size_t alloc;
    size_t length;
    struct Page *pages;
    size_t cur;
    bool valid;
};

void Slide_init(struct Slide *slide);
void Slide_push(struct Slide *slide, struct Page page);
void Slide_cleanup(struct Slide *slide);
void Slide_prev(struct Slide *slide);
void Slide_next(struct Slide *slide);
void Slide_parse(struct Slide *slide, char *path, bool simple);
struct Page *Slide_current_page(struct Slide *slide);

#endif /* SSLIDE_H */

