#include "Frame.h"
#include "Global.h"
#include "Font.h"
#include "Log.h"
#include "Rect.h"
#include "String.h"
#include "Math.h"
#include <SDL2/SDL_ttf.h>

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
        String joined = StringArray_join(lines, "\n");
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
    char *path = config.font ? config.font : frame->font;
    int bl = 0, br = 256;
    int linecount = StringArray_length(frame->lines);
    int lfac = config.linespacing * (linecount - 1);
    while (bl <= br) {
        int m = bl + (br - bl) / 2;
        int longest_line_w = 0, line_h;
        TTF_Font *font = TTF_OpenFont(path, m);
        if (!font) {
            Warn("Failed to open font %s: %s", path, TTF_GetError());
            return -1;
        }
        for (int i = 0; i < linecount; i++) {
            int line_w;
            TTF_SizeUTF8(font, frame->lines[i], &line_w, &line_h);
            longest_line_w = maxi(longest_line_w, line_w);
        }
        if (line_h * linecount + lfac < frame->h * r->height / 100 &&
            longest_line_w < frame->w * r->width / 100) {
            *size = m;
            *text_width = longest_line_w;
            *text_height = line_h * linecount + lfac;
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
    int framew = frame->w * r->width / 100;
    int frameh = frame->h * r->height / 100;
    if (frame->type == FrameText && StringArray_length(frame->lines)) {
        int total_width = 0, total_height = 0, fontsize = 0;
        if (Frame_find_font_size(frame, r, &fontsize, &total_width,
                                 &total_height) != 0) {
            frame->valid = false;
            return;
        }
        TTF_Font *font = TTF_OpenFont(config.font ? config.font : frame->font, fontsize);
        int line_height = TTF_FontHeight(font);
        int xoffset = (framew - total_width) / 2;
        int yoffset = (frameh - total_height) / 2;

        for (size_t i = 0; i < StringArray_length(frame->lines); i++) {
            SDL_Surface *textsurface =
                TTF_RenderUTF8_Blended(font, frame->lines[i], config.fg);
            SDL_Texture *texttexture =
                SDL_CreateTextureFromSurface(r->rend, textsurface);
            int linew, lineh;
            SDL_QueryTexture(texttexture, NULL, NULL, &linew, &lineh);
            SDL_RenderCopy(r->rend, texttexture, NULL, &(SDL_Rect) {
                .x = frame->x * r->width / 100 + xoffset,
                .y = frame->y * r->height / 100 + (line_height + config.linespacing) * i +
                     yoffset,
                .w = linew,
                .h = lineh,
            });
            SDL_FreeSurface(textsurface);
            SDL_DestroyTexture(texttexture);
        }
        TTF_CloseFont(font);
    } else if (frame->type == FrameImage) {
        Image_draw(&frame->image, r,
                &(struct Rect) {frame->x * r->width / 100, frame->y * r->height / 100,
                        frame->w * r->width / 100, frame->h * r->height / 100});
    }
}


