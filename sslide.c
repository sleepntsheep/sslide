/* sslide - sleepntsheep 2022
 *
 * I am ashamed for extensive use of global variable here, forgive me
 *
 */

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_rwops.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_video.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#define SHEEP_DYNARRAY_IMPLEMENTATION
#include "dynarray.h"
#ifndef NO_JSON_CONFIG
#define SHEEP_SJSON_IMPLEMENTATION
#include "sjson.h"
#define SHEEP_LOG_IMPLEMENTATION
#include "log.h"
#endif
#define SHEEP_XMALLOC_IMPLEMENTATION
#include "xmalloc.h"
#include "config.h"
#include "tinyfiledialogs.h"

#ifndef NO_UNIFONT
#include "unifont.h"
#else
#include "font.h"
#endif

#ifdef _WIN32
#include <windows.h>
#include <shlwapi.h>
#else
#include <unistd.h>
#include <pwd.h>
#endif

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define die(...) _die(__LINE__, __VA_ARGS__)

static void _die(int line_number, const char * format, ...)
{
    fprintf(stderr, "%d: ", line_number);
    va_list vargs;
    va_start (vargs, format);
    vfprintf(stderr, format, vargs);
    va_end (vargs);
    fprintf(stderr, "\n");
    abort();
}

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
    int x,y,w,h;
} Frame;

typedef Frame* Page;
typedef Page*  Slide;

static int w, h;
static SDL_RWops *fontrw;
static SDL_Window *win;
static SDL_Renderer *rend;
static TTF_Font *fonts[FONT_NSCALES+1];
static int fontsheight[FONT_NSCALES+1];
static const int linespacing = 3;
static Slide slide;
static char *fontpath = NULL;
/* work directory, if reading from file
 * this is directory which that file is in
 * else if reading from stdin it's NULL */
static char *slidewd = NULL;
static bool progressbar = true;
static bool invert = false;

#ifndef NO_JSON_CONFIG
void readconfig();
#endif
void loadfonts();
Slide parse_slide_from_file(FILE *in);
int getfontsize(Frame frame, int *width, int *height);
void init();
void drawframe(Frame frame);
void run();
void cleanup();
char *gethomedir();

void loadfonts() {
    if (fontpath == NULL) {
        fontrw = SDL_RWFromMem(font_ttf, font_ttf_len);
        if (fontrw == NULL) {
            die("FontRwop: %s", SDL_GetError());
        }
    }
    for (int i = 1; i <= FONT_NSCALES; i++) {
        if (fontpath == NULL) {
            SDL_RWseek(fontrw, 0, RW_SEEK_SET);
            fonts[i] = TTF_OpenFontRW(fontrw, false, i * FONT_STEP);
        } else {
            fonts[i] = TTF_OpenFont(fontpath, i * FONT_STEP);
        }
        if (fonts[i] == NULL) {
            fprintf(stderr, "Open Font fail (size: %d): %s\n", i, SDL_GetError());
        }
        fontsheight[i] = TTF_FontHeight(fonts[i]);
    }
}

int getfontsize(Frame frame, int *width, int *height) {
    /* may be faster with binary search */
    int result = 0;
    int linecount = arrlen(frame.lines);
    int lfac = linespacing * (linecount - 1);
    for (int i = FONT_NSCALES; i > 0; i--) {
        if (fontsheight[i] * linecount + lfac < frame.h * h / 100) {
            result = i;
            break;
        }
    }
    int longesti = 0;
    int longestw = 0;
    for (size_t i = 0; i < arrlen(frame.lines); i++) {
        int linew, _h;
        TTF_SizeUTF8(fonts[result], frame.lines[i], &linew, &_h);
        if (linew > longestw) {
            longestw = linew;
            longesti = i;
        }
    }
    for (int i = result; i > 0; i--) {
        int linewidth, _h;
        TTF_SizeUTF8(fonts[i], frame.lines[longesti], &linewidth, &_h);
        if (linewidth < frame.w * w / 100) {
            *width = linewidth;
            result = i;
            break;
        }
    }
    *height = fontsheight[result] * linecount + lfac;
    return result;
}

void init() {
    w = 800;
    h = 600;
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    if (IMG_Init(IMG_INIT_AVIF | IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_WEBP) < 1) {
        die("Img_INIT: %s", IMG_GetError());
    }
    win = SDL_CreateWindow("Slide", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, SDL_WINDOW_RESIZABLE);
    rend = SDL_CreateRenderer(win, -1, 0);
}

void cleanup() {
    if (fontrw != NULL) {
        SDL_RWclose(fontrw);
    }
    for (int i = 1; i <= FONT_NSCALES; i++) {
        TTF_CloseFont(fonts[i]);
    }
    for (size_t i = 0; i < arrlen(slide); i++) {
        for (size_t j = 0; j < arrlen(slide[i]); j++) {
            if (slide[i][j].type == FRAMEIMAGE) {
                SDL_DestroyTexture(slide[i][j].image.texture);
            } else {
                arrfree(slide[i][j].lines);
            }
        }
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
        int twidth, theight;
        int fontsize = getfontsize(frame, &twidth, &theight);
        int ptperline = linespacing + fontsheight[fontsize];
        int xoffset = (framew - twidth) / 2;
        int yoffset = (frameh - theight) / 2;

        for (size_t i = 0; i < arrlen(frame.lines); i++) {
            SDL_Surface *textsurface = TTF_RenderUTF8_Blended(fonts[fontsize],
                    frame.lines[i], invert ? bg : fg);
            SDL_Texture *texttexture = SDL_CreateTextureFromSurface(rend, textsurface);
            int linew, lineh;
            SDL_QueryTexture(texttexture, NULL, NULL, &linew, &lineh);
            SDL_Rect bound = {
                .x = frame.x * w / 100 + xoffset,
                .y = frame.y * h / 100 + ptperline * i + yoffset,
                .w = linew,
                .h = lineh,
            };
            SDL_RenderCopy(rend, texttexture, NULL, &bound);
            SDL_FreeSurface(textsurface);
            SDL_DestroyTexture(texttexture);
        }
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
    for (size_t i = 0; i < arrlen(page); i++) {
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
    size_t pagei = 0;
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
                drawprogressbar((float)(pagei+1)/arrlen(slide));
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
        if (argv[1][0] == '-') {
            fin = stdin;
        } else {
            srcfile = argv[1];
        }
    } else {
        srcfile = (char*)tinyfd_openFileDialog("Open slide", gethomedir(), 0, NULL, NULL, false);
    }

    if (srcfile != NULL /* not reading from stdin */) {
        fin = fopen(srcfile, "r");
        if (fin == NULL) {
            die("Failed opening file %s", srcfile);
        }
        /* basename() is not portable */
        char *sep = srcfile + strlen(srcfile);
        while (sep > srcfile && 
                *sep != '/'
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

#ifndef NO_JSON_CONFIG
    readconfig();
#endif
    init();
    loadfonts();
    slide = parse_slide_from_file(fin);
    run();
    cleanup();
}

Slide parse_slide_from_file(FILE *in) {
    Slide slide = arrnew(Page);
    size_t slidewdlen = slidewd ? strlen(slidewd) : 0;
    char buf[SSLIDE_BUFSIZE] = { 0 };
    while (true) {
        Page page = arrnew(Frame);
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
                if (arrlen(slide) == 0) {
                    die("Slide has no page, make sure every page is terminated");
                }
                /* didn't add page to slide, must free now to prevent memory leak */
                arrfree(page);
                return slide;
            }

            /* geo */
            if (buf[0] == '@') {
                break;
            }
            if (buf[0] != ';') {
                die("No geometry before frame");
            }
            if (buf[1] == 'f') {
                frame.x = frame.y = 0;
                frame.w = frame.h = 100;
            } else if (sscanf(buf, ";%d;%d;%d;%d", &frame.x, &frame.y, &frame.w, &frame.h) != 4) {
                die("Geometry need to be in ;x;y;w;h");
			}

            while (fgets(buf, sizeof buf, in) != NULL /* EOF */) {
                if (buf[0] == '%') {
                    if (frame.type == FRAMEIMAGE) {
                        die("Only one image per frame is allowed");
                    }
                    char *newline = strchr(buf, '\n');
                    if (newline != NULL) {
                        *newline = '\x0';
                    } else {
                        die("Error parsing");
                    }
                    char filename[PATH_MAX] = { 0 };
                    if (in == stdin
#ifndef _WIN32 /* path beginning with root is absolute path */
                            || buf[1] == '/'
#else /* relative path in windows is complicated */
                            || !PathIsRelative(buf+1)
#endif
                            ) {
                        strcpy(filename, buf+1);
                    } else {
                        size_t basenamelen = strlen(buf+1);
                        memcpy(filename, slidewd, slidewdlen);
                        filename[slidewdlen] = '/';
                        memcpy(filename + slidewdlen + 1, buf+1, basenamelen);
                        filename[slidewdlen + 1 + basenamelen] = '\x0';
                    }

                    frame.type = FRAMEIMAGE;
                    frame.image.texture = IMG_LoadTexture(rend, filename);
                    if (frame.image.texture == NULL) {
                        fprintf(stderr, "Failed loading image %s: %s", filename, SDL_GetError());
                    }
                    int imgw, imgh;
                    SDL_QueryTexture(frame.image.texture, NULL, NULL, &imgw, &imgh);
                    frame.image.ratio = (float)imgw / imgh;
                } else if (buf[0] != '\n') {
                    if (frame.lines == NULL) {
                        frame.lines = arrnew(char*);
                    }
                    if (buf[0] == '#') {
                        /* comment */
                        continue;
                    }
                    if (frame.type == FRAMEIMAGE) {
                        die("Text and image is same frame is not allowed");
                    }
                    frame.type = FRAMETEXT;
                    int blen = strlen(buf);
                    buf[blen-1] = '\x0'; /* truncate newline character */
                    char *bufalloc = xmalloc(blen+1);
                    memcpy(bufalloc, buf, blen);
                    arrpush(frame.lines, bufalloc);
                } else /* empty line, terminating frame */ {
                    arrpush(page, frame);
                    break;
                }
            }
        }
        arrpush(slide, page);
    }
    return slide;
}

#ifndef NO_JSON_CONFIG
void readconfig() {
    char configpath[PATH_MAX] = { 0 };
    strcpy(configpath, gethomedir());
    strcat(configpath, "/.sslide.json");
    FILE *configfile = fopen(configpath, "ab+");
    fseek(configfile, 0L, SEEK_END);
    size_t fsize = ftell(configfile);
    fseek(configfile, 0L, SEEK_SET);
    char *content = xmalloc(fsize+1);
    fread(content, 1, fsize, configfile);
    content[fsize] = '\x0';
    fclose(configfile);
    sjson_result sj = sjson_deserialize(content, fsize);
    if (sj.err) goto bad;
    if (sj.json->type != SJSON_OBJECT) goto bad;
    sjson_result fontpathjson = sjson_object_get(sj.json, "fontpath");
    if (!fontpathjson.err)  {
        if (fontpathjson.json->type == SJSON_STRING) {
            fontpath = fontpathjson.json->v.str;
        }
    }
    sjson_free(sj.json);
    free(content);
bad:
    warn("Failed reading json config");
    return;
}
#endif

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

