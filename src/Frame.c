#include "Frame.h"
#include "Global.h"
#include "Font.h"
#include "Log.h"
#include "Rect.h"
#include "String.h"
#include "Math.h"
#include <SDL2/SDL_ttf.h>

// x * r->width / 100
// x * ((100 + magrinx) * r->width / 100) / 100
// rx = fx (r->width + marginx r->width/100) / 100
struct Rect Frame_get_rect_px(const struct Frame *frame, const struct Renderer *r) {
    struct Rect rm = {
        .x = r->width * config.margin_x / 100,
        .y = r->height * config.margin_y / 100,
        .w = r->width - 2 * rm.x,
        .h = r->height - 2 * rm.y,
    };

    return (struct Rect) {
        .x = rm.x + frame->x * rm.w / 100,
        .y = rm.y + frame->y * rm.h / 100,
        .w = frame->w * rm.w / 100,
        .h = frame->h * rm.h / 100,
    };
}

void Frame_text_init(struct Frame *frame, StringArray lines, int x, int y, int w,
                    int h) {
    frame->type = FrameText;
    frame->x = x;
    frame->y = y;
    frame->w = w;
    frame->h = h;
    frame->lines = lines;
    frame->valid = true;
    if (lines) {
        String joined = StringArray_join(lines, "");
        if (!config.font) {
            frame->font = Font_covering_ttf(joined);
            if (frame->font == NULL)
                frame->valid = false;
        } else {
            frame->font = config.font;
        }
        String_free(joined);
    }
}

void Frame_image_init(struct Frame *frame, struct Image image, int x, int y, int w, int h) {
    frame->type = FrameImage;
    frame->image = image;
    frame->x = x;
    frame->y = y;
    frame->w = w;
    frame->h = h;
    frame->valid = true;
}

void Frame_cleanup(struct Frame *frame) {
    if (frame->type == FrameText) {
        StringArray_free(frame->lines);
    } else if (frame->type == FrameImage) {
        Image_cleanup(&frame->image);
    }
}

int Frame_find_font_size(struct Frame *frame, struct Renderer *r, int *size, int *text_width,
                         int *text_height) {
    /* binary search to find largest font size that fit in frame */
    const struct Rect fr = Frame_get_rect_px(frame, r);
    char *path = config.font ? config.font : frame->font;
    int bl = 0, br = 256;
    int linecount = StringArray_length(frame->lines);
    int lfac = config.linespacing * (linecount - 1);

    while (bl <= br) {
        int m = bl + (br - bl) / 2;
        int text_w = 0, text_h = 0, line_h = 0, line_w = 0;
        TTF_Font *font = TTF_OpenFont(path, m);
        if (!font) {
            Warn("Failed to open font %s: %s", path, TTF_GetError());
            return -1;
        }
        for (int i = 0; i < linecount; i++) {
            if (TTF_SizeUTF8(font, frame->lines[i], &line_w, &line_h) == -1) {
                Warn("Failed to get size: %s", TTF_GetError());
            }
            text_w = maxi(text_w, line_w);
            text_h += line_h;
        }
        text_h += lfac;
        if (text_h <= fr.h && text_w <= fr.w) {
            *size = m;
            *text_width = text_w,
            *text_height = text_h,
            bl = m + 1;
        } else {
            br = m - 1;
        }
        TTF_CloseFont(font);
    }
    return 0;
}

void Frame_draw(struct Frame *frame, struct Renderer *r) {
    if (!frame->valid) return;
    const struct Rect fr = Frame_get_rect_px(frame, r);
    if (frame->type == FrameText && StringArray_length(frame->lines)) {
        int text_width = 0, text_height = 0, fontsize = 0;
        if (Frame_find_font_size(frame, r, &fontsize, &text_width,
                                 &text_height) != 0) {
            frame->valid = false;
            return;
        }
        TTF_Font *font = TTF_OpenFont(config.font ? config.font : frame->font, fontsize);
        int line_height = TTF_FontHeight(font);
        int xoffset = (fr.w - text_width) / 2;
        int yoffset = (fr.h - text_height) / 2;

        for (size_t i = 0; i < StringArray_length(frame->lines); i++) {
            SDL_Surface *textsurface =
                TTF_RenderUTF8_Blended(font, frame->lines[i], config.fg);
            SDL_Texture *texttexture =
                SDL_CreateTextureFromSurface(r->rend, textsurface);
            int linew, lineh;
            SDL_QueryTexture(texttexture, NULL, NULL, &linew, &lineh);
            SDL_RenderCopy(r->rend, texttexture, NULL, &(SDL_Rect) {
                .x = fr.x + xoffset,
                .y = fr.y + (line_height + config.linespacing) * i +
                     yoffset,
                .w = linew,
                .h = lineh,
            });
            SDL_FreeSurface(textsurface);
            SDL_DestroyTexture(texttexture);
        }
        TTF_CloseFont(font);
    } else if (frame->type == FrameImage) {
        Image_draw(&frame->image, r, fr);
    }
}


