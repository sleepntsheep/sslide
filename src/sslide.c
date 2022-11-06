/* sslide - sleepntsheep 2022
 *
 * I am ashamed for extensive use of global variable here, forgive me
 *
 */
#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <errno.h>
#include <libgen.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#include <fontconfig/fontconfig.h>

#include "config.h"
#include "font.h"
#include "path.h"
#include "tinyfiledialogs.h"
#define SHEEP_DYNARRAY_IMPLEMENTATION
#include "dynarray.h"
#define SHEEP_FMT_IMPLEMENTATION
#include "attr.h"
#include "fmt.h"

#define VERSION "0.0.7"

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

enum FrameType {
    FrameNone,
    FrameText,
    FrameImage,
};

typedef struct {
    SDL_Texture *texture;
    char path[PATH_MAX];
    double ratio;
    bool valid;
} Image;

typedef struct {
    int type;
    Image image;
    char **lines;
    char font[PATH_MAX];
    int x, y, w, h;
    bool valid;
} Frame;

typedef Frame *Page;
typedef Page *Slide;

static SDL_Window *win = NULL;
static SDL_Renderer *rend = NULL;
static Slide slide = dynarray_new;
static int linespacing = 3;
static int width = 800, height = 600;
static bool progressbar = true;
static bool invert = false;

int slide_from_file(Slide *, char *, bool);
void init(const char *title);
void drawprogressbar(double);
void run(void);
void cleanup(void);

void image_init(Image *, char *);
void image_load_texture(Image *, SDL_Renderer *);
void image_draw(Image *, SDL_Renderer *, Frame *);
void image_cleanup(Image *);
void frame_cleanup(Frame *);

void frametext_init(Frame *, dynarray(char *), int, int, int, int);
void frameimage_init(Frame *, Image, int, int, int, int);
void frame_draw(Frame *, SDL_Renderer *);
int frame_find_font_size(const Frame *const, int *, int *, int *);

void image_init(Image *image, char *path) {
    image->valid = true;
    image->texture = NULL;
    image->ratio = 1;
    strcpy(image->path, path);
}

void image_load_texture(Image *image, SDL_Renderer *rend) {
    if (!image->valid)
        return;
    if (image->texture)
        return;
    image->texture = IMG_LoadTexture(rend, image->path);
    int w = 0, h = 0;
    if (image->texture == NULL) {
        ffmt(stderr, "Failed loading image {str}: {str}", image->path,
             IMG_GetError());
        image->valid = false;
    } else {
        SDL_QueryTexture(image->texture, NULL, NULL, &w, &h);
    }
    image->ratio = (double)w / h;
}

void image_cleanup(Image *image) {
    if (image->valid && image->texture)
        SDL_DestroyTexture(image->texture);
}

void frametext_init(Frame *frame, dynarray(char *) lines, int x, int y, int w,
                    int h) {
    frame->type = FrameText;
    frame->x = x;
    frame->y = y;
    frame->w = w;
    frame->h = h;
    frame->lines = lines;
    frame->valid = true;
    size_t lines_len = 0;
    for (size_t i = 0; i < arrlen(lines); i++)
        lines_len += strlen(lines[i]);
    char *joined = calloc(lines_len + 1, 1);
    size_t offset = 0;
    for (size_t i = 0; i < arrlen(lines); i++) {
        size_t line_len = strlen(lines[i]);
        memcpy(joined + offset, lines[i], line_len);
        offset += line_len;
    }
    int res = get_best_ttf(joined, frame->font, sizeof frame->font);
    if (res != 0 || !frame->font[0])
        frame->valid = false;
    free(joined);
}

void frameimage_init(Frame *frame, Image image, int x, int y, int w, int h) {
    frame->type = FrameImage;
    frame->image = image;
    frame->x = x;
    frame->y = y;
    frame->w = w;
    frame->h = h;
    frame->valid = true;
}

void frame_cleanup(Frame *frame) {
    if (frame->type == FrameText) {
        if (frame->lines) {
            for (size_t i = 0; i < arrlen(frame->lines); i++)
                free(frame->lines[i]);
            arrfree(frame->lines);
        }
    } else if (frame->type == FrameImage) {
        image_cleanup(&frame->image);
    }
}

int frame_find_font_size(const Frame *const frame, int *size, int *text_width,
                         int *text_height) {
    /* binary search to find largest font size that fit in frame */
    int bl = 0, br = 256;
    int linecount = dynarray_len(frame->lines);
    int lfac = linespacing * (linecount - 1);
    while (bl <= br) {
        int m = bl + (br - bl) / 2;
        int longest_line_w = 0, line_h;
        TTF_Font *font = TTF_OpenFont(frame->font, m);
        if (!font) {
            ffmt(stderr, "Failed to open font: {str}", TTF_GetError());
            return -1;
        }
        for (size_t i = 0; i < dynarray_len(frame->lines); i++) {
            int line_w;
            TTF_SizeUTF8(font, frame->lines[i], &line_w, &line_h);
            longest_line_w = MAX(longest_line_w, line_w);
        }
        if (line_h * linecount + lfac < frame->h * height / 100 &&
            longest_line_w < frame->w * width / 100) {
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

void init(const char *title) {
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
#ifdef IMG_INIT_AVIF
    if (IMG_Init(IMG_INIT_AVIF | IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_WEBP) <
        1)
#else
    if (IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_WEBP) < 1)
#endif
        ffmt(stderr, "Img_INIT: {str}", IMG_GetError());
    win =
        SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                         width, height, SDL_WINDOW_RESIZABLE);
    if (!win)
        ffmt(stderr, "Failed creating window: {str}", SDL_GetError());
    rend = SDL_CreateRenderer(win, -1, 0);
    if (!rend)
        ffmt(stderr, "Failed creating renderer: {str}", SDL_GetError());
    ffmt(stderr, "Initialized window\n");
}

void cleanup(void) {
    for (size_t i = 0; i < arrlen(slide); i++) {
        for (size_t j = 0; j < arrlen(slide[i]); j++)
            frame_cleanup(&slide[i][j]);
        arrfree(slide[i]);
    }
    if (slide)
        arrfree(slide);
    SDL_Quit();
    TTF_Quit();
    IMG_Quit();
}

void image_draw(Image *image, SDL_Renderer *rend, Frame *frame) {
    if (!image->valid)
        return;
    int framew = frame->w * width / 100;
    int frameh = frame->h * height / 100;
    int framex = frame->x * width / 100;
    int framey = frame->y * height / 100;
    image_load_texture(image, rend);
    SDL_Rect bound;
    /* find maximum (width, height) which
     * width <= .width, height <= .height
     * and width / height == ratio */
    int hbyw = framew / image->ratio;
    int wbyh = frameh * image->ratio;
    if (hbyw <= height) {
        bound.w = framew;
        bound.h = hbyw;
    }
    if (wbyh <= width) {
        bound.w = wbyh;
        bound.h = frameh;
    }
    int xoffset = (framew - bound.w) / 2;
    int yoffset = (frameh - bound.h) / 2;
    bound.x = framex + xoffset;
    bound.y = framey + yoffset;
    SDL_RenderCopy(rend, image->texture, NULL, &bound);
}

void frame_draw(Frame *frame, SDL_Renderer *rend) {
    int framew = frame->w * width / 100;
    int frameh = frame->h * height / 100;
    if (frame->type == FrameText) {
        int total_width, total_height, fontsize;
        if (frame_find_font_size(frame, &fontsize, &total_width,
                                 &total_height) != 0) {
            return;
        }
        TTF_Font *font = TTF_OpenFont(frame->font, fontsize);
        int line_height = TTF_FontHeight(font);
        int xoffset = (framew - total_width) / 2;
        int yoffset = (frameh - total_height) / 2;

        for (size_t i = 0; i < arrlen(frame->lines); i++) {
            SDL_Surface *textsurface =
                TTF_RenderUTF8_Blended(font, frame->lines[i], invert ? bg : fg);
            SDL_Texture *texttexture =
                SDL_CreateTextureFromSurface(rend, textsurface);
            int linew, lineh;
            SDL_QueryTexture(texttexture, NULL, NULL, &linew, &lineh);
            SDL_Rect dest_rect = {
                .x = frame->x * width / 100 + xoffset,
                .y = frame->y * height / 100 + (line_height + linespacing) * i +
                     yoffset,
                .w = linew,
                .h = lineh,
            };
            SDL_RenderCopy(rend, texttexture, NULL, &dest_rect);
            SDL_FreeSurface(textsurface);
            SDL_DestroyTexture(texttexture);
        }
        TTF_CloseFont(font);
    } else if (frame->type == FrameImage) {
        image_draw(&frame->image, rend, frame);
    }
}

void page_draw(Page page, SDL_Renderer *rend) {
    SDL_Color realbg = invert ? fg : bg;
    SDL_SetRenderDrawColor(rend, realbg.r, realbg.g, realbg.b, realbg.a);
    SDL_RenderClear(rend);
    for (size_t i = 0; i < arrlen(page); i++)
        frame_draw(page + i, rend);
}

void drawprogressbar(double progress /* value between 0 and 1 */) {
    SDL_Color realfg = invert ? bg : fg;
    SDL_SetRenderDrawColor(rend, realfg.r, realfg.g, realfg.b, realfg.a);
    SDL_RenderFillRect(rend, &(SDL_Rect){
                                 .x = 0,
                                 .y = height - PROGRESSBAR_HEIGHT,
                                 .w = progress * width,
                                 .h = PROGRESSBAR_HEIGHT,
                             });
}

void run(void) {
    if (arrlen(slide) == 0)
        return;
    size_t pagei = 0;
    SDL_Event event;

    while (true) {
        bool redraw = false;
        /* event loop */
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                return;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                case SDLK_0:
                    FALLTHROUGH;
                case SDLK_1:
                case SDLK_2:
                case SDLK_3:
                case SDLK_4:
                case SDLK_5:
                case SDLK_6:
                case SDLK_7:
                case SDLK_8:
                case SDLK_9:
                    pagei = (event.key.keysym.sym - SDLK_0) *
                            (arrlen(slide) - 1) / 9.0L;
                    redraw = true;
                    break;
                case SDLK_UP:
                    FALLTHROUGH;
                case SDLK_LEFT:
                case SDLK_k:
                    if (pagei > 0) {
                        pagei--;
                        redraw = true;
                    }
                    break;
                case SDLK_DOWN:
                    FALLTHROUGH;
                case SDLK_RIGHT:
                case SDLK_j:
                    if (pagei < arrlen(slide) - 1) {
                        pagei++;
                        redraw = true;
                    }
                    break;
                case SDLK_u:
                    pagei = 0;
                    redraw = true;
                    break;
                case SDLK_d:
                    pagei = arrlen(slide) - 1;
                    redraw = true;
                    break;
                case SDLK_i:
                    invert = !invert;
                    redraw = true;
                    break;
                case SDLK_p:
                    progressbar = !progressbar;
                    redraw = true;
                    break;
                default:
                    break;
                }
                break;
            case SDL_WINDOWEVENT:
                switch (event.window.event) {
                case SDL_WINDOWEVENT_RESIZED:
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                case SDL_WINDOWEVENT_MAXIMIZED:
                    width = event.window.data1;
                    height = event.window.data2;
                    redraw = true;
                    break;
                case SDL_WINDOWEVENT_MOVED:
                case SDL_WINDOWEVENT_EXPOSED:
                    redraw = true;
                    break;
                default:
                    break;
                }
                break;
            default:
                break;
            }
        }

        if (redraw) {
            page_draw(slide[pagei], rend);
            if (progressbar)
                drawprogressbar((double)(pagei + 1) / arrlen(slide));
            SDL_RenderPresent(rend);
        }
        SDL_Delay(15);
    }
}

int slide_from_file(Slide *slide, char *path, bool simple) {
    FILE *in = NULL;
    int line = 0;

    if (strcmp(path, "-") != 0) {
        in = fopen(path, "r");
        if (!in) {
            ffmt(stderr, "Failed opening file {str}: {str}", path,
                 strerror(errno));
            return -1;
        }
    } else {
        in = stdin;
    }

    char *path_dup = strdup(path);
    char *path_dir = dirname(path_dup);
    bool valid = true;

    *slide = dynarray_new;
    char buf[SSLIDE_BUFSIZE] = {0};

    for (;;) {
        Page page = dynarray_new;
        for (;;) {
            Image image = {0};
            char *ret = NULL;
            dynarray(char *) lines = dynarray_new;
            int type = FrameNone;

            /* clear empty lines */
            for (; (ret = fgets(buf, sizeof buf, in)); line++)
                if (buf[0] != '\n' && buf[0] != '#')
                    break;
            char *nl = strchr(buf, '\n');
            if (nl)
                *nl = 0;

            if (ret == NULL /* EOF */) {
                if (arrlen(*slide) == 0) {
                    if (simple) {
                        arrpush(*slide, page);
                    } else {
                        ffmt(stderr,
                             "Slide has no page, make sure every page is "
                             "terminated with a @\n");
                        arrfree(page);
                        valid = false;
                        goto end_slide;
                    }
                }
                goto end_slide;
            }

            if (!strcmp(buf, "@")) {
                if (simple)
                    goto next_page_skip_current;
                else
                    goto next_page;
            }

            int x = 0, y = 0, w = 100, h = 100;
            if (sscanf(buf, ";%d;%d;%d;%d", &x, &y, &w, &h) != 4 &&
                strcmp(buf, ";f") != 0) {
                if (simple) {
                    arrpush(lines, strdup(buf));
                } else {
                    ffmt(stderr, "Wrong geo format line: {int} ({str})\n", line,
                         buf);
                    valid = false;
                }
            }

            for (; fgets(buf, sizeof buf, in) != NULL; line++) {
                char *nl = strchr(buf, '\n');
                if (nl)
                    *nl = 0;

                if (buf[0] == '#')
                    continue;
                if (buf[0] == '%') {
                    if (type == FrameImage)
                        ffmt(stderr,
                             "Only one image per frame is allowed (line {int})",
                             line);
                    type = FrameImage;
                    char path[PATH_MAX * 2 + 1];
                    if (!strcmp(path_dir, ".") || !path_is_relative(buf + 1))
                        strcpy(path, buf + 1);
                    else
                        snprintf(path, sizeof path, "%s/%s", path_dir, buf + 1);
                    image_init(&image, path);
                } else if (buf[0]) {
                    type = FrameText;
                    arrpush(lines, strdup(buf));
                } else {
                    break;
                }
            }

            Frame nf = {0};
            if (type == FrameText) {
                if (lines == NULL)
                    nf.valid = false;
                else
                    frametext_init(&nf, lines, x, y, w, h);
            } else {
                frameimage_init(&nf, image, x, y, w, h);
            }
            if (nf.valid)
                arrpush(page, nf);

            if (simple)
                goto next_page;
        }
    next_page:
        arrpush(*slide, page);
    next_page_skip_current:;
    }
end_slide:
    free(path_dup);
    if (in != stdin)
        fclose(in);
    if (!valid)
        *slide = NULL;
    return valid ? 0 : 1;
}

int main(int argc, char **argv) {
    fmt_set_flush(true);

    char *srcfile = NULL;
    bool simple = false;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-v")) {
            ffmt(stderr, "{str} Version: {str}\n", argv[0], VERSION);
            exit(0);
        } else if (!strcmp(argv[i], "-s")) {
            simple = true;
        } else {
            srcfile = argv[i];
        }
    }

    if (!srcfile) {
        char dir[PATH_MAX] = {0};
        if (path_home_dir(dir, sizeof dir) != 0) {
            strcpy(dir, ".");
        }
        srcfile = (char *)tinyfd_openFileDialog("Open slide", dir, 0, NULL,
                                                NULL, false);
        if (!srcfile) {
            ffmt(stderr, "Usage: {str} [OPTIONS] <FILE>\n", argv[0]);
            return 0;
        }
    } else if (!strcmp(srcfile, "-")) {
        ffmt(stderr, "Reading from stdin\n");
    }

    if (slide_from_file(&slide, srcfile, simple) != 0)
        return 1;
    init(srcfile);
    run();
    cleanup();
    return 0;
}
