#ifndef TOUCH_H
#define TOUCH_H

#include <SDL.h>
#include "common.h"

class Touch
{

public:
    static Touch *instance();
    ~Touch();

    void handleEvent(SDL_Event *e);
    int getDragScreenOffsetX();
    int getDragScreenOffsetY();
    bool isScreenMoving();
    void reset();
    void main();
    void setXBoundaries(int32_t, int32_t);

protected:
    Touch();
    static Touch *m_instance;

    Point dragStartPos, dragStartPosWithOffset;
    Point currentMousePos;
    int dragVerticalLength;
    int dragHorizontalLength;
    int dragScreenOffsetX, dragScreenOffsetY;
    bool isDragging;
    bool mouseButtonPressed;
    int accelerationX, accelerationY;
    uint32_t dragStartTime;

    int32_t boundary1x, boundary2x;
};

#endif
