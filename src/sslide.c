/* sslide - sleepntsheep 2022
 *
 * I am ashamed for extensive use of global variable here, forgive me
 *
 */
#define _POSIX_C_SOURCE 200809L
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_rwops.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_video.h>
#include <fontconfig/fontconfig.h>

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#define SHEEP_DYNARRAY_IMPLEMENTATION
#include "dynarray.h"
#define SHEEP_LOG_IMPLEMENTATION
#include "log.h"
#define SHEEP_XMALLOC_IMPLEMENTATION
#include "config.h"
#include "tinyfiledialogs.h"
#include "xmalloc.h"
#include "font.h"
#include "compat/path.h"
#include "compat/mem.h"

#define INITIAL_SIZE 4
#define VERSION "0.0.6"

enum FrameType {
    FRAMETEXT,
    FRAMEIMAGE,
    FRAMETYPECOUNT,
};

typedef struct {
    SDL_Texture *texture;
    float ratio;
} Image;

typedef struct {
    int type;
    Image image;
    char **lines;
    char *font;
    int x, y, w, h;
} Frame;

typedef Frame *Page;
typedef Page *Slide;

/*
typedef struct {
    Frame *frames;
    size_t len;
    size_t cap;
} Page;

typedef struct {
    Page *pages;
    size_t len;
    size_t cap;
} Slide;

void image_cleanup(Image *image)
{
    if (image->texture != NULL)
        SDL_DestroyTexture(image->texture);
}

void frame_init(Frame *frame, int x, int y, int w, int h)
{
    frame->x = x;
    frame->y = y;
    frame->w = w;
    frame->h = h;
    frame->lines = StrArray_new();
}

void frame_cleanup(Frame *frame)
{
    image_cleanup(frame_cleanup);
    StrArray_free(frame->lines);
    free(font);
}

void page_init(Page *page)
{
    page->len = 0;
    page->cap = INITIAL_SIZE;
    page->frames = malloc(sizeof *page->frames * page->cap);
}

void page_push_frame(Page *page, Frame frame)
{
    if (page->len == page->cap)
    {
        page->cap *= 2;
        page->frames = realloc(page->frames, sizeof *page->frames
                page->cap);
    }
    page->frames[page->len++] = frame;
}

void page_cleanup(Page *page)
{
    for (size_t i = 0; i < page->len; i++)
        frame_cleanup(page->frames[i]);
    free(page->frames);
}
*/

typedef struct {
    Slide slide;
    int width;
    int height;
    int line_spacing;
    bool progress_bar;
    bool invert;
    SDL_Window *window;
    SDL_Renderer *renderer;
} Context;

static SDL_Window *win;
static SDL_Renderer *rend;
static const int linespacing = 3;
static int w = 800, h = 600;
static Slide slide = dynarray_new;
static bool progressbar = true;
static bool invert = false;
static FontManager font_manager;

Slide parse_slide_from_file(char *path);
int getfontsize(Frame frame, int *width, int *height, char *path);
void init();
void drawframe(Frame frame);
void run();
void cleanup();

int getfontsize(Frame frame, int *width, int *height, char *path) {
    /* binary search to find largest font size that fit in frame */
    int bl = 0, br = 256;
    int linecount = dynarray_len(frame.lines);
    int lfac = linespacing * (linecount - 1);
    int result = 0;
    while (bl <= br) {
        int m = bl + (br - bl) / 2;
        int longest_line_w = 0, line_h;
        TTF_Font *font = TTF_OpenFont(path, m);
        for (long i = 0; i < dynarray_len(frame.lines); i++) {
            int line_w;
            TTF_SizeUTF8(font, frame.lines[i], &line_w, &line_h);
            if (line_w > longest_line_w)
                longest_line_w = line_w;
        }
        if (line_h * linecount + lfac < frame.h * h / 100 &&
                longest_line_w < frame.w * w / 100) {
            result = m;
            *width = longest_line_w;
            *height = line_h * linecount + lfac;
            bl = m + 1;
        } else {
            br = m - 1;
        }
        TTF_CloseFont(font);
    }
    return result;
}

void init() {
    w = 800;
    h = 600;
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    if (IMG_Init(IMG_INIT_AVIF | IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_WEBP) < 1)
        panic("Img_INIT: %s", IMG_GetError());
    win = SDL_CreateWindow("Slide", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, SDL_WINDOW_RESIZABLE);
    if (!win)
        panic("Failed creating window: %s", SDL_GetError());
    rend = SDL_CreateRenderer(win, -1, 0);
    if (!rend)
        panic("Failed creating renderer: %s", SDL_GetError());
    if (FontManager_init(&font_manager) < 0)
        panic("Failed initializing FontManager");
}

void cleanup() {
    for (long i = 0; i < dynarray_len(slide); i++) {
        for (long j = 0; j < dynarray_len(slide[i]); j++) {
            if (slide[i][j].type == FRAMEIMAGE) {
                SDL_DestroyTexture(slide[i][j].image.texture);
            }
            else {
                for (long k = 0; k < dynarray_len(slide[i][j].lines); k++) {
                    free(slide[i][j].lines[k]);
                }
                arrfree(slide[i][j].lines);
            }
            free(slide[i][j].font);
        }
        arrfree(slide[i]);
    }
    FontManager_cleanup(&font_manager);
    arrfree(slide);
    SDL_Quit();
    TTF_Quit();
    IMG_Quit();
}

void drawframe(Frame frame) {
    int framex = frame.x * w / 100;
    int framey = frame.y * h / 100;
    int framew = frame.w * w / 100;
    int frameh = frame.h * h / 100;
    if (frame.type == FRAMETEXT) {
        int total_width, total_height;
        int fontsize = getfontsize(frame, &total_width, &total_height, frame.font);
        TTF_Font *font = TTF_OpenFont(frame.font, fontsize);
        int line_height = TTF_FontHeight(font);
        int xoffset = (framew - total_width) / 2;
        int yoffset = (frameh - total_height) / 2;

        for (long i = 0; i < dynarray_len(frame.lines); i++) {
            SDL_Surface *textsurface = TTF_RenderUTF8_Blended(
                font, frame.lines[i], invert ? bg : fg);
            SDL_Texture *texttexture =
                SDL_CreateTextureFromSurface(rend, textsurface);
            int linew, lineh;
            SDL_QueryTexture(texttexture, NULL, NULL, &linew, &lineh);
            SDL_Rect bound = {
                .x = frame.x * w / 100 + xoffset,
                .y = frame.y * h / 100 + (line_height + linespacing) * i + yoffset,
                .w = linew,
                .h = lineh,
            };
            SDL_RenderCopy(rend, texttexture, NULL, &bound);
            SDL_FreeSurface(textsurface);
            SDL_DestroyTexture(texttexture);
        }
        TTF_CloseFont(font);
    } else if (frame.type == FRAMEIMAGE) {
        SDL_Rect bound;
        /* find maximum (width, height) which
         * width <= frame.width, height <= frame.height
         * and width / height == ratio */
        int hbyw = framew / frame.image.ratio;
        int wbyh = frameh * frame.image.ratio;
        if (hbyw <= frameh) {
            bound.w = framew;
            bound.h = hbyw;
        }
        if (wbyh <= framew) {
            bound.w = wbyh;
            bound.h = frameh;
        }
        int xoffset = (framew - bound.w) / 2;
        int yoffset = (frameh - bound.h) / 2;
        bound.x = framex + xoffset;
        bound.y = framey + yoffset;
        SDL_RenderCopy(rend, frame.image.texture, NULL, &bound);
    }
}

void drawpage(Page page) {
    SDL_Color realbg = invert ? fg : bg;
    SDL_SetRenderDrawColor(rend, realbg.r, realbg.g, realbg.b, realbg.a);
    SDL_RenderClear(rend);
    for (long i = 0; i < dynarray_len(page); i++) {
        drawframe(page[i]);
    }
}

void drawprogressbar(float progress /* value between 0 and 1 */) {
    SDL_Rect bar = {
        .x = 0,
        .y = h - PROGRESSBAR_HEIGHT,
        .w = progress * w,
        .h = PROGRESSBAR_HEIGHT,
    };
    SDL_Color realfg = invert ? bg : fg;
    SDL_SetRenderDrawColor(rend, realfg.r, realfg.g, realfg.b, realfg.a);
    SDL_RenderFillRect(rend, &bar);
}

void run() {
    long pagei = 0;
    SDL_Event event;

    while (true) {
        bool redraw = false;
        /* event loop */
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                exit(0);
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                case SDLK_UP: /* FALLTHROUGH */
                case SDLK_LEFT:
                case SDLK_k:
                    if (pagei > 0) {
                        pagei--;
                        redraw = true;
                    }
                    break;
                case SDLK_DOWN: /* FALLTHROUGH */
                case SDLK_RIGHT:
                case SDLK_j:
                    if (pagei < dynarray_len(slide) - 1) {
                        pagei++;
                        redraw = true;
                    }
                    break;
                case SDLK_u:
                    pagei = 0;
                    redraw = true;
                    break;
                case SDLK_d:
                    pagei = dynarray_len(slide) - 1;
                    redraw = true;
                    break;
                case SDLK_i:
                    invert ^= true;
                    redraw = true;
                    break;
                case SDLK_p:
                    progressbar ^= true;
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
                    SDL_GetRendererOutputSize(rend, &w, &h);
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
            drawpage(slide[pagei]);
            if (progressbar) {
                drawprogressbar((float)(pagei + 1) / dynarray_len(slide));
            }
            SDL_RenderPresent(rend);
        }
        SDL_Delay(15);
    }
}

int main(int argc, char **argv) {
    char *srcfile = NULL;
    if (argc > 1) {
        if (strcmp(argv[1], "-v") == 0) {
            printf("%s Version: %s\n", argv[0], VERSION);
            exit(0);
        }
        else {
            if (strcmp(argv[1], "-") == 0)
                info("Reading from stdin");
            srcfile = argv[1];
        }
    } else {
        srcfile = (char *)tinyfd_openFileDialog("Open slide", path_home_dir(), 0, NULL, NULL, false);
    }

    init();
    slide = parse_slide_from_file(srcfile);
    if (arrlen(slide) != 0) run();

    cleanup();
    return 0;
}

Slide parse_slide_from_file(char *path) {
    FILE *in;
    if (!strcmp(path, "-")) in = stdin;
    else in = fopen(path, "r");

    char path_dir[PATH_MAX];
    path_dirname(path, strlen(path), path_dir, sizeof path_dir);
    size_t path_dir_len = strlen(path_dir);

    Slide slide = dynarray_new;
    char buf[SSLIDE_BUFSIZE] = {0};
    while (true) {
        Page page = dynarray_new;
        while (true) {
            Frame frame = {0};
            char *ret;

            /* clear empty lines */
            while ((ret = fgets(buf, sizeof buf, in))) {
                if (!(buf[0] == '\n' || buf[0] == '#')) {
                    break;
                }
            }

            if (ret == NULL /* EOF */) {
                if (dynarray_len(slide) == 0) {
                    warn("Slide has no page, make sure every page is "
                         "terminated with a @");
                }
                /* didn't add page to slide, must free now to prevent memory
                 * leak */
                dynarray_free(page);
                return slide;
            }

            /* geo */
            if (buf[0] == '@') {
                break;
            }
            if (buf[0] != ';') {
                panic("No geometry before frame (;x;y;w;h)");
            }
            if (buf[1] == 'f') {
                frame.x = frame.y = 0;
                frame.w = frame.h = 100;
            } else if (sscanf(buf, ";%d;%d;%d;%d", &frame.x, &frame.y, &frame.w,
                              &frame.h) != 4) {
                panic("Geometry need to be in ;x;y;w;h");
            }

            while (fgets(buf, sizeof buf, in) != NULL /* EOF */) {
                if (buf[0] == '%') {
                    if (frame.type == FRAMEIMAGE) {
                        warn("Only one image per frame is allowed");
                    }
                    char *newline = strchr(buf, '\n');
                    if (newline != NULL) {
                        *newline = '\x0';
                    } else {
                        warn("Error parsing, don't know why");
                    }
                    char filename[PATH_MAX] = {0};
                    if (in == stdin || !path_is_relative(buf + 1)) {
                        strcpy(filename, buf + 1);
                    } else {
                        long basenamelen = strlen(buf + 1);
                        memcpy(filename, path_dir, path_dir_len);
                        filename[path_dir_len] = '/';
                        memcpy(filename + path_dir_len + 1, buf + 1, basenamelen);
                        filename[path_dir_len + 1 + basenamelen] = '\x0';
                    }

                    frame.type = FRAMEIMAGE;
                    frame.image.texture = IMG_LoadTexture(rend, filename);
                    if (frame.image.texture == NULL) {
                        warn("Failed loading image %s: %s", filename,
                             IMG_GetError());
                    }
                    int imgw, imgh;
                    SDL_QueryTexture(frame.image.texture, NULL, NULL, &imgw,
                                     &imgh);
                    frame.image.ratio = (float)imgw / imgh;
                } else if (buf[0] != '\n') {
                    if (buf[0] == '#') {
                        continue;
                    }
                    if (frame.type == FRAMEIMAGE) {
                        panic("Text and image is same frame is not allowed");
                    }
                    frame.type = FRAMETEXT;
                    char *bufalloc = strdup(buf);
                    *strchr(bufalloc, '\n') = 0;
                    dynarray_push(frame.lines, bufalloc);
                } else /* empty line, terminating frame */ {
                    frame.font = FontManager_get_best_font(&font_manager, frame.lines, arrlen(frame.lines));
                    dynarray_push(page, frame);
                    break;
                }
            }
        }
        dynarray_push(slide, page);
    }
    if (in != stdin) fclose(in);
    return slide;
}

