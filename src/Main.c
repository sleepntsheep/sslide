/* sslide - sleepntsheep 2022
 *
 * I am ashamed for extensive use of global variable here, forgive me
 *
 */
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fontconfig/fontconfig.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#include "tinyfiledialogs.h"

#include "Log.h"
#include "Config.h"
#include "Frame.h"
#include "Font.h"
#include "String.h"
#include "Global.h"
#include "Image.h"
#include "Path.h"
#include "Page.h"
#include "Slide.h"

#define VERSION "0.0.15"

void drawprogressbar(struct Renderer *r, float progress /* value between 0 and 1 */) {
    if (config.progress_bar_height == 0) return;
    SDL_SetRenderDrawColor(r->rend, config.fg.r, config.fg.g, config.fg.b, config.fg.a);
    SDL_RenderFillRect(r->rend, &(SDL_Rect){
            .x = 0,
            .y = r->height - config.progress_bar_height,
            .w = progress * r->width,
            .h = config.progress_bar_height,
    });
}

void run(struct Renderer *r, struct Slide *slide) {
    if (slide->length == 0)
        return;
    SDL_Event event;

    bool redraw = true;
    while (true) {
        /* event loop */
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                return;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                case SDLK_0:
                case SDLK_1:
                case SDLK_2:
                case SDLK_3:
                case SDLK_4:
                case SDLK_5:
                case SDLK_6:
                case SDLK_7:
                case SDLK_8:
                case SDLK_9:
                    slide->cur = (event.key.keysym.sym - SDLK_0) *
                            (slide->length - 1) / 9.0L;
                    redraw = true;
                    break;
                case SDLK_UP:
                case SDLK_LEFT:
                case SDLK_k:
                    Slide_prev(slide);
                    redraw = true;
                    break;
                case SDLK_DOWN:
                case SDLK_RIGHT:
                case SDLK_j:
                    Slide_next(slide);
                    redraw = true;
                    break;
                case SDLK_u:
                    slide->cur = 0;
                    redraw = true;
                    break;
                case SDLK_d:
                    slide->cur = slide->length - 1;
                    redraw = true;
                    break;
                case SDLK_i: {
                    SDL_Color tmp = config.bg;
                    config.bg = config.fg;
                    config.fg = tmp;
                    redraw = true;
                    break;
                             }
                default:
                    break;
                }
                break;
            case SDL_WINDOWEVENT:
                switch (event.window.event) {
                case SDL_WINDOWEVENT_RESIZED:
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                case SDL_WINDOWEVENT_MAXIMIZED:
                    r->width = event.window.data1;
                    r->height = event.window.data2;
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
            Page_draw(Slide_current_page(slide), r);
            drawprogressbar(r, (float)(slide->cur + 1) / slide->length);
            SDL_RenderPresent(r->rend);
            redraw = false;
        }
        SDL_Delay(15);
    }
}

int main(int argc, char **argv) {
    Log_global_init();

    char *src = NULL;
    bool simple = false;

    Info("Sslide version %s", VERSION);

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-s")) {
            simple = true;
        } else {
            src = argv[i];
        }
    }

    if (!src) {
        String home = Path_gethome();
        if (!home) home = ".";
        src = tinyfd_openFileDialog("Open slide", home, 0, 0, 0, false);
        String_free(home);
        if (!src) {
            Info("Usage: %s [OPTIONS] <FILE>\n", argv[0]);
            return 0;
        }
    } else if (!strcmp(src, "-")) {
        Info("Reading from stdin");
    }

    struct Slide slide;
    Slide_parse(&slide, src, simple);
    if (!slide.valid)
		Panic("Failed parsing slide, aborting");

    Renderer_global_init();
    struct Renderer renderer;
    Renderer_init(&renderer, src);
    run(&renderer, &slide);
    Slide_cleanup(&slide);
    Renderer_cleanup(&renderer);
    Renderer_global_cleanup();

    return EXIT_SUCCESS;
}

