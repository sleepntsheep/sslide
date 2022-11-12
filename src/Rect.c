#include "Rect.h"

struct Rect Rect_make(int x, int y, int w, int h) {
    return (struct Rect) {
        x, y, w, h
    };
}

