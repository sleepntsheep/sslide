#pragma once
#include "String.h"
#include "Image.h"

enum FrameType {
    FrameNone,
    FrameText,
    FrameImage,
};

struct Frame {
    int type;
    struct Image image;
    String *lines;
    char *font;
    int x, y, w, h;
    bool valid;
};

void Frame_cleanup(struct Frame *frame);
void Frame_text_init(struct Frame *frame, char **lines, int, int, int, int);
void Frame_image_init(struct Frame *frame, struct Image image, int, int, int, int);
void Frame_draw(struct Frame *frame, struct Renderer *r);
int Frame_find_font_size(struct Frame *frame, struct Renderer *r, int *size, int *text_width,
                         int *text_height);

