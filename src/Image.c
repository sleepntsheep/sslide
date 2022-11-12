#include <string.h>
#include <SDL2/SDL_image.h>
#include "Image.h"
#include "Rect.h"
#include "Log.h"

void Image_init(struct Image *image, char *path) {
    image->valid = true;
    image->texture = NULL;
    image->ratio = 1;
    strcpy(image->path, path);
}

void Image_load_texture(struct Image *image, struct Renderer *r) {
    if (!image->valid || image->texture)
        return;
    image->texture = IMG_LoadTexture(r->rend, image->path);
    int w = 0, h = 0;
    if (image->texture == NULL) {
        Warn("Failed loading image %s: %s", image->path,
             IMG_GetError());
        image->valid = false;
    } else {
        SDL_QueryTexture(image->texture, NULL, NULL, &w, &h);
    }
    image->ratio = (float)w / h;
}

void Image_cleanup(struct Image *image) {
    if (image->valid && image->texture)
        SDL_DestroyTexture(image->texture);
}

void Image_draw(struct Image *image, struct Renderer *r, const struct Rect *rect) {
    if (!image->valid)
        return;
    Image_load_texture(image, r);
    SDL_Rect bound;
    /* find maximum (width, height) which
     * width <= .width, height <= .height
     * and width / height == ratio */
    int hbyw = rect->w / image->ratio;
    int wbyh = rect->h * image->ratio;
    if (hbyw <= rect->h) {
        bound.w = rect->w;
        bound.h = hbyw;
    }
    if (wbyh <= rect->w) {
        bound.w = wbyh;
        bound.h = rect->h;
    }
    int xoffset = (rect->w - bound.w) / 2;
    int yoffset = (rect->h - bound.h) / 2;
    bound.x = rect->x + xoffset;
    bound.y = rect->y + yoffset;
    SDL_RenderCopy(r->rend, image->texture, NULL, &bound);
}


