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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "font.h"
#define SHEEP_DYNARRAY_IMPLEMENTATION
#include "dynarray.h"
#define SHEEP_SJSON_IMPLEMENTATION
#include "sjson.h"
#define SHEEP_XMALLOC_IMPLEMENTATION
#include "xmalloc.h"
#include "config.h"

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))

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
static const SDL_Color bg = { 255, 255, 255, 255 };
static const SDL_Color fg = { 0,   0,   0,   255 };
static Slide slide;
static char *fontpath = NULL;

void readconfig();
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
        fontrw = SDL_RWFromMem(font_uint8, font_uint8_len);
        if (fontrw == NULL) {
            fprintf(stderr, "FontRwop: %s\n", SDL_GetError());
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
            fprintf(stderr, "Open Font fail %d: %s\n", i, SDL_GetError());
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
    for (int i = 0; i < arrlen(frame.lines); i++) {
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
        fprintf(stderr, "IMG_Init Failed: %s", IMG_GetError());
        exit(1);
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

        for (int i = 0; i < arrlen(frame.lines); i++) {
            SDL_Surface *textsurface = TTF_RenderUTF8_Blended(fonts[fontsize],
                    frame.lines[i], fg);
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
    SDL_SetRenderDrawColor(rend, bg.r, bg.g, bg.b, bg.a);
    SDL_RenderClear(rend);
    for (size_t i = 0; i < arrlen(page); i++) {
        drawframe(page[i]);
    }
    SDL_RenderPresent(rend);
}

void run() {
    size_t pagei = 0;
    SDL_Event event;
    drawpage(slide[pagei]);

    while (1) {
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
                        default:
                            break;
                    }
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
                default:
                    break;
            }
        }

        if (redraw) {
            drawpage(slide[pagei]);
        }
        SDL_Delay(15);
    }
}

int main(int argc, char **argv) {
    FILE *fin = stdin;
    if (argc > 1) {
        fin = fopen(argv[1], "r");
    }
    readconfig();
    init();
    loadfonts();
    slide = parse_slide_from_file(fin);
    run();
    cleanup();
}

Slide parse_slide_from_file(FILE *in) {
    Slide slide = arrnew(Page);
    char buf[SSLIDE_BUFSIZE] = { 0 };
    while (true) {
        Page page = arrnew(Frame);
        while (true) {
            Frame frame = {
                .lines = arrnew(char*),
            };
            char *ret;

            /* clear empty lines */
            while ((ret = fgets(buf, sizeof buf, in))) {
                if (!(buf[0] == '\n' || buf[0] == '#')) {
                    break;
                }
            }
            if (ret == NULL /* EOF */) {
                if (arrlen(slide) == 0) {
                    fprintf(stderr, "Error parsing: Slide has no page, make sure every page is terminated");
                    exit(1);
                }
                return slide;
            }

            /* geo */
            if (buf[0] == '@') {
                break;
            }
            if (buf[0] != ';') {
                fprintf(stderr, "Wrong slide format: no geometry before frame");
                exit(1);
            }
            if (buf[1] == 'f') {
                frame.x = frame.y = 0;
                frame.w = frame.h = 100;
            } else {
                int ret;
                ret = sscanf(buf, ";%d;%d;%d;%d", &frame.x, &frame.y, &frame.w, &frame.h);
                if (ret != 4) {
                    fprintf(stderr, "Wrong slide format: Geometry need to be in ;x;y;w;h");
                    exit(1);
                }
            }

            while (fgets(buf, sizeof buf, in) != NULL /* EOF */) {
                if (buf[0] == '%') {
                    if (frame.type == FRAMEIMAGE) {
                        fprintf(stderr, "Only one image per frame is allowed!");
                        exit(1);
                    }
                    char *filename = buf+1;
                    char *newline = strchr(filename, '\n');
                    if (newline != NULL) {
                        *newline = '\x0';
                    } else {
                        fprintf(stderr, "Error parsing");
                        exit(1);
                    }
                    frame.type = FRAMEIMAGE;
                    frame.image.texture = IMG_LoadTexture(rend, filename);
                    if (frame.image.texture == NULL) {
                        fprintf(stderr, "Failed loading image %s: %s", filename, SDL_GetError());
                        exit(1);
                    }
                    int imgw, imgh;
                    SDL_QueryTexture(frame.image.texture, NULL, NULL, &imgw, &imgh);
                    frame.image.ratio = (float)imgw / imgh;
                } else if (buf[0] != '\n') {
                    if (buf[0] == '#') {
                        /* comment */
                        continue;
                    }
                    if (frame.type == FRAMEIMAGE) {
                        fprintf(stderr, "Text and image in same frame is not allowed!");
                        exit(1);
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

void readconfig() {
    char configpath[4096] = { 0 };
    strcpy(configpath, gethomedir());
    strcat(configpath, "/.sslide.json");
    puts(configpath);
    FILE *configfile = fopen(configpath, "ab+");
    fseek(configfile, 0L, SEEK_END);
    size_t fsize = ftell(configfile);
    fseek(configfile, 0L, SEEK_SET);
    char *content = xmalloc(fsize+1);
    fread(content, 1, fsize, configfile);
    content[fsize] = '\x0';
    fclose(configfile);
    sjson *json = sjson_serialize(content, fsize);
    if (json == NULL) goto bad;
    if (json->type != SJSON_OBJECT) goto bad;
    sjson *fontpathjson = sjson_object_get(json, "fontpath");
    if (fontpathjson != NULL)  {
        if (fontpathjson->type != SJSON_STRING) goto bad;
        fontpath = fontpathjson->v.str;
    }
    sjson_free(json);
    free(content);
    return;
bad:
    fprintf(stderr, "No/Bad config file, Skipping\n");
    return;
}

char *gethomedir() {
    char *ret = NULL;
#ifdef _WIN32
    ret = getenv("USERPROFILE");
    if (ret == NULL) {
        ret = xcalloc(1, 4096);
        strcat(ret, getenv("HOMEDRIVE"));
        strcat(ret, getenv("HOMEPATH"));
    }
#else
#include <unistd.h>
#include <pwd.h>
    ret = getenv("HOME");
    if (ret == NULL) {
        ret = getpwuid(getuid())->pw_dir;
    }
#endif
    return ret;
}

