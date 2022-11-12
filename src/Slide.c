#include <errno.h>
#include "Slide.h"
#include "Global.h"
#include "Path.h"
#include "Log.h"

#define SSLIDE_BUFSIZE 8192 /* max line length */

void Slide_init(struct Slide *slide) {
    slide->alloc = 4;
    slide->length = 0;
    slide->pages = malloc(sizeof(struct Page) * slide->alloc);
    slide->cur = 0;
}

void Slide_push(struct Slide *slide, struct Page page) {
    if (slide->length == slide->alloc)
        slide->pages = realloc(slide->pages, sizeof (struct Page) * (slide->alloc *= 2));
    slide->pages[slide->length++] = page;
}

void Slide_cleanup(struct Slide *slide) {
    for (size_t i = 0; i < slide->length; i++)
        Page_cleanup(&slide->pages[i]);
    free(slide->pages);
}

void Slide_next(struct Slide *slide) {
    if (slide->cur < slide->length - 1)
        slide->cur++;
}

void Slide_prev(struct Slide *slide) {
    if (slide->cur > 0)
        slide->cur--;
}

struct Page *Slide_current_page(struct Slide *slide) {
    return slide->pages + slide->cur;
}

void Slide_parse(struct Slide *slide, char *path, const bool simple) {
    FILE *in = NULL;
    int line = 0;
    slide->valid = true;
    struct Page page = {0};
    struct Image image = {0};
    int x = 0, y = 0, w = 100, h = 100;
    int type = FrameNone;
    int in_config = 0;

    StringArray lines = StringArray_new();
    StringArray flines = StringArray_new();
    String text = String_make("");
    size_t fsize = 0;
    size_t flinecount = 1;

    char *path_dir = NULL;
    if (in == stdin) {
        path_dir = Path_dirname(".");
    } else {
        path_dir = Path_dirname(path);
    }

    if (strcmp(path, "-") != 0) {
        if (!(in = fopen(path, "r"))) {
            Warn("Failed opening file %s: %s", path, strerror(errno));
            slide->valid = false;
            goto cleanup;
        }
    } else {
        in = stdin;
    }

    if (in != stdin) {
        fseek(in, 0L, SEEK_END);
        fsize = ftell(in);
        fseek(in, 0L, SEEK_SET);
        text = String_realloc(text, fsize + 1);
        if (fread(text, fsize, 1, in) != 1) {
            Warn("Failed reading file data: %s", strerror(errno));
        }
        text[fsize] = 0;
    } else {
        char buf[SSLIDE_BUFSIZE];
        while (fgets(buf, sizeof buf, in)) {
            text = String_cat(text, buf);
        }
        fsize = String_length(text);
    }

    for (size_t i = 0; i <= fsize; i++) {
        if (text[i] == '\n') {
            text[i] = 0;
            flinecount++;
        }
    }

    Info("Read input file with %d lines", flinecount + 1);

    /* note - there might be more NUL terminator than flinecount,
     * so mallocing char** while would be faster is not safe */
    flines = StringArray_push(flines, text);
    for (size_t i = 0; i < fsize; i++) {
        if (text[i] == 0) {
            flines = StringArray_push(flines, text + i + 1);
        }
    }

    config.simple = simple;
    Info("Parsing file %s : simple: %s", path, simple ? "true" : "false");

    Slide_init(slide);
    Page_init(&page);

    for (size_t li = 0; li < StringArray_length(flines); li++) {
        char *cline = flines[li];
        if (cline[0] == '#') {
            continue;
        }
        else if (!strcmp(cline, "---")) {
            in_config++;
        }
        else if (in_config == 1) {
            config_parse_line(&config, cline);
        }
        else if (cline[0] == '\x0' || (!strcmp(cline, "@") && type != FrameNone)) {
            struct Frame nf = {0};
            if (type == FrameText) {
                Frame_text_init(&nf, lines, x, y, w, h);
                lines = StringArray_new();
            } else {
                Frame_image_init(&nf, image, x, y, w, h);
                image = (struct Image){0};
            }
            if (nf.valid) {
                Page_push(&page, nf);
            } else {
                Warn("Failed to init new frame");
            }
            if (config.simple || !strcmp(cline, "@")) {
                Slide_push(slide, page);
                Page_init(&page);
            }
            x = y = 0;
            w = h = 100;
            type = FrameNone;
        }
        else if (!strcmp(cline, "@")) {
            Slide_push(slide, page);
            Page_init(&page);
        }
        else if (cline[0] == ';' && type == FrameNone) {
            x = 0, y = 0, w = 100, h = 100;
            if (sscanf(cline, ";%d;%d;%d;%d", &x, &y, &w, &h) != 4 &&
                    strcmp(cline, ";f")) {
                if (config.simple) {
                    lines = StringArray_push(lines, cline);
                } else {
                    Warn("Wrong geo format line: %s (%s)", line, cline);
                    slide->valid = false;
                }
            }
            type = FrameNone;
        }
        else if (cline[0] == '%') {
            String path = NULL;
            if (!strcmp(path_dir, ".") || !Path_isrelative(cline + 1))
                path = String_make(cline + 1);
            else
                path = String_format("%s/%s", path_dir, cline + 1);
            Image_init(&image, path);
            String_free(path);
            type = FrameImage;
        } else {
            lines = StringArray_push(lines, cline);
            type = FrameText;
        }
    }

    if (slide->length == 0) {
        if (config.simple) {
            Slide_push(slide, page);
        } else {
            Warn("Slide has no page, make sure every page is"
                    " terminated with a @");
            slide->valid = false;
        }
    }
    if (lines != NULL) {
        StringArray_free(lines);
    }

cleanup:
    if (path_dir) String_free(path_dir);
    Page_cleanup(&page);
    if (flines) StringArray_free(flines);
    if (text) String_free(text);
    if (in != stdin)
        fclose(in);
}


