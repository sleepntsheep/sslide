#pragma once
#ifndef RECT_H_
#define RECT_H_

struct Rect {
    int x, y, w, h;
};

struct Rect Rect_make(int x, int y, int w, int h);

#endif /* RECT_H_ */
