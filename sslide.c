/* sslide - sleepntsheep 2022
 *
 * I am ashamed for extensive use of global variable here, forgive me
 *
 * TODO
 *   - fallback font support, maybe with fontconfig
 *     not sure how scaling would work since different fonts
 *     have different sizes
 */

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
#define SHEEP_DYNARRAY_IMPLEMENTATION
#include "dynarray.h"
#define SHEEP_LOG_IMPLEMENTATION
#include "log.h"
#define SHEEP_XMALLOC_IMPLEMENTATION
#include "config.h"
#include "tinyfiledialogs.h"
#include "xmalloc.h"

#ifdef USE_UNIFONT
#include "unifont.h"
#else
#include "font.h"
#endif

#ifdef _WIN32
/* needed for tinyfiledialog and PathIsRelative */
#include <shlwapi.h>
#include <windows.h>
#else
/* needed for getting home directory (getpwuid)
   in case $HOME isn't set */
#include <pwd.h>
#include <unistd.h>
#endif

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

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

static FcConfig *fc_config = NULL;
static FcFontSet *fc_all_font = NULL;
static SDL_Window *win;
static SDL_Renderer *rend;
static const int linespacing = 3;
static int w = 800, h = 600;
static Slide slide = dynarray_new;
/* work directory, if reading from file
 * this is directory which that file is in
 * else if reading from stdin it's NULL */
static char *slidewd = NULL;
static bool progressbar = true;
static bool invert = false;

Slide parse_slide_from_file(FILE *in);
int getfontsize(Frame frame, int *width, int *height, char *path);
void init();
void drawframe(Frame frame);
void run();
void cleanup();
char *gethomedir();

char *frame_best_font(char **text) {
    FcCharSet *cs = FcCharSetCreate();
    for (long i = 0; i < dynarray_len(text); i++) {
        size_t len = strlen(text[i]);
        for (size_t j = 0; j < len && text[i][j];) {
            FcChar32 cs4;
            int utf8len = FcUtf8ToUcs4((const FcChar8*)text[i] + j, &cs4, len - j);
            j += utf8len;
            FcCharSetAddChar(cs, cs4);
        }
    }
    FcPattern *pat = FcPatternBuild(NULL, FC_CHARSET, FcTypeCharSet, cs, NULL);

    FcResult match_result = 0;
    FcFontSet *matches = FcFontSort(fc_config, pat, true, &cs, &match_result);

    FcCharSetDestroy(cs);
    if (pat) FcPatternDestroy(pat);

    for (int i = 0; i < matches->nfont; i++) {
        FcChar8 *path;
        FcResult get_result = FcPatternGetString(matches->fonts[i], FC_FILE, 0, &path);
        if (get_result != 0) continue;
        if (FcStrStr(path, (const FcChar8*)".ttf") == NULL) {
            continue;
        }
        FcFontSetDestroy(matches);
        return (char*)path;
    }

    if (matches) FcFontSetDestroy(matches);
    return NULL;
}

int getfontsize(Frame frame, int *width, int *height, char *path) {
    /* binary search to find largest font size that fit in frame */
    int bl = 0, br = 128;
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
    win = SDL_CreateWindow("Slide", SDL_WINDOWPOS_CENTERED,
                           SDL_WINDOWPOS_CENTERED, w, h, SDL_WINDOW_RESIZABLE);
    rend = SDL_CreateRenderer(win, -1, 0);
    if (!FcInit())
        panic("FcInit Failed");
    fc_config = FcConfigGetCurrent();
    FcPattern *pat = FcPatternCreate();
    FcObjectSet *os = FcObjectSetBuild(FC_FAMILY, FC_STYLE, FC_FILE, NULL);
    fc_all_font = FcFontList(fc_config, pat, os);
}

void cleanup() {
    for (long i = 0; i < dynarray_len(slide); i++) {
        for (long j = 0; j < dynarray_len(slide[i]); j++)
            if (slide[i][j].type == FRAMEIMAGE)
                SDL_DestroyTexture(slide[i][j].image.texture);
            else
                arrfree(slide[i][j].lines);
        arrfree(slide[i]);
    }
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
    FILE *fin = NULL;
    char *srcfile = NULL;
    if (argc > 1) {
        if (strcmp(argv[1], "-") == 0) {
            fin = stdin;
            info("Reading from stdin");
        } else {
            srcfile = argv[1];
        }
    } else {
        srcfile = (char *)tinyfd_openFileDialog("Open slide", gethomedir(), 0, NULL, NULL, false);
    }

    if (srcfile != NULL /* not reading from stdin */) {
        fin = fopen(srcfile, "r");
        if (fin == NULL) {
            panicerr("Failed opening file %s", srcfile);
        }
        /* basename() is not portable */
        char *sep = srcfile + strlen(srcfile);
        while (sep > srcfile && *sep != '/'
#ifdef _WIN32
               && *sep != '\\'
#endif
        )
            sep--;
        if (sep > srcfile) {
            *sep = 0;
            slidewd = srcfile;
        } else {
            slidewd = ".";
        }
    }

    init();
    slide = parse_slide_from_file(fin);
    if (arrlen(slide) != 0) run();
    cleanup();
}

Slide parse_slide_from_file(FILE *in) {
    Slide slide = dynarray_new;
    long slidewdlen = slidewd ? strlen(slidewd) : 0;
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
                    if (in == stdin
#ifndef _WIN32 /* path beginning with root is absolute path */
                        || buf[1] == '/'
#else /* relative path in windows is complicated */
                        || !PathIsRelative(buf + 1)
#endif
                    ) {
                        strcpy(filename, buf + 1);
                    } else {
                        long basenamelen = strlen(buf + 1);
                        memcpy(filename, slidewd, slidewdlen);
                        filename[slidewdlen] = '/';
                        memcpy(filename + slidewdlen + 1, buf + 1, basenamelen);
                        filename[slidewdlen + 1 + basenamelen] = '\x0';
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
                        /* comment */
                        continue;
                    }
                    if (frame.type == FRAMEIMAGE) {
                        panic("Text and image is same frame is not allowed");
                    }
                    frame.type = FRAMETEXT;
                    int blen = strlen(buf);
                    buf[blen - 1] = '\x0'; /* truncate newline character */
                    char *bufalloc = xmalloc(blen + 1);
                    memcpy(bufalloc, buf, blen);
                    dynarray_push(frame.lines, bufalloc);
                } else /* empty line, terminating frame */ {
                    frame.font = frame_best_font(frame.lines);
                    dynarray_push(page, frame);
                    break;
                }
            }
        }
        dynarray_push(slide, page);
    }
    return slide;
}

char *gethomedir() {
    char *ret = NULL;
#ifdef _WIN32
    ret = getenv("USERPROFILE");
    if (ret == NULL) {
        ret = xcalloc(1, PATH_MAX);
        strcat(ret, getenv("HOMEDRIVE"));
        strcat(ret, getenv("HOMEPATH"));
    }
#else
    ret = getenv("HOME");
    if (ret == NULL) {
        ret = getpwuid(getuid())->pw_dir;
    }
#endif
    return ret;
}

