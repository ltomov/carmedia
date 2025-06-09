#include "touch.h"

// TODO: remove the vertical calculations if not using them. for performance stuff

Touch *Touch::instance()
{
    if (!m_instance)
        m_instance = new Touch;

    return m_instance;
}

Touch::Touch()
{
    dragStartPos.x = 0;
    dragStartPos.y = 0;
    dragStartPosWithOffset.x = 0;
    dragStartPosWithOffset.y = 0;
    dragVerticalLength = 0;
    dragHorizontalLength = 0;
    isDragging = false;
    mouseButtonPressed = false;
    accelerationX = 0;
    accelerationY = 0;
    dragStartTime = 0;
    dragScreenOffsetX = 0;
    dragScreenOffsetY = 0;

    boundary1x = 0;
    boundary2x = 0;
}

Touch::~Touch()
{
    dragStartPos.x = 0;
    dragStartPos.y = 0;
    dragVerticalLength = 0;
    isDragging = false;
}


void Touch::handleEvent(SDL_Event *e)
{
    if (e->type == SDL_MOUSEBUTTONDOWN)
    {
        mouseButtonPressed = true;

        SDL_GetMouseState(&dragStartPos.x, &dragStartPos.y);
        dragStartPosWithOffset.x = dragStartPos.x - dragScreenOffsetX;
        dragStartPosWithOffset.y = dragStartPos.y - dragScreenOffsetY;

        dragVerticalLength = 0;
        dragHorizontalLength = 0;

        accelerationX = 0;
        accelerationY = 0;
        dragStartTime = SDL_GetTicks();
    }
    else if (e->type == SDL_MOUSEMOTION)
    {
        SDL_GetMouseState(&currentMousePos.x, &currentMousePos.y);

        if (mouseButtonPressed && !isDragging && (abs(currentMousePos.x - dragStartPos.x) > 2 || abs(currentMousePos.y - dragStartPos.y) > 2))
            isDragging = true;
    }
    else if (e->type == SDL_MOUSEBUTTONUP)
    {
        mouseButtonPressed = false;

        if (isDragging)
        {
            dragVerticalLength = currentMousePos.y - dragStartPos.y;
            dragHorizontalLength = currentMousePos.x - dragStartPos.x;
        }

        isDragging = false;

        dragStartPos.x = 0;
        dragStartPos.y = 0;

        uint32_t timeSpentDragging = SDL_GetTicks() - dragStartTime;
        int velocityX = abs(dragHorizontalLength) / ((timeSpentDragging / 2) + 1);
        int velocityY = abs(dragVerticalLength) / ((timeSpentDragging / 2) + 1);

        if (dragHorizontalLength > 20)
            accelerationX = velocityX * 50;
        else if (dragHorizontalLength < -20)
            accelerationX = -(velocityX * 50);


        if (dragVerticalLength > 20)
            accelerationY = velocityY * 50;
        else if (dragVerticalLength < -20)
            accelerationY = -(velocityY * 50);
    }
}

bool Touch::isScreenMoving()
{
    return (isDragging || accelerationX || accelerationX);
}

int Touch::getDragScreenOffsetX()
{
    return dragScreenOffsetX;
}

int Touch::getDragScreenOffsetY()
{
    return dragScreenOffsetY;
}

void Touch::main()
{
    if (accelerationX > 0)
        accelerationX--;
    else if (accelerationX < 0)
        accelerationX++;

    if (accelerationY > 0)
        accelerationY--;
    else if (accelerationY < 0)
        accelerationY++;

    if (dragScreenOffsetX > boundary1x || dragScreenOffsetX < -boundary2x)
    {
        accelerationX = 0;
        accelerationY = 0;
    }

    if (isDragging)
    {
        int _dragScreenOffsetX = currentMousePos.x - dragStartPosWithOffset.x;
        if (_dragScreenOffsetX < boundary1x && _dragScreenOffsetX > -boundary2x)
            dragScreenOffsetX = _dragScreenOffsetX;

        dragScreenOffsetY = currentMousePos.y - dragStartPosWithOffset.y;
    }

    dragScreenOffsetX += (accelerationX / 7);
    dragScreenOffsetY += (accelerationY / 7);

}

void Touch::reset()
{
    accelerationX = 0;
    accelerationY = 0;

    dragScreenOffsetX = 0;
    dragScreenOffsetY = 0;

    boundary1x = 0;
    boundary2x = 0;
}

void Touch::setXBoundaries(int32_t b1, int32_t b2)
{
    boundary1x = b1;
    boundary2x = b2;
}
