#ifndef BUTTON_H
#define BUTTON_H

#include <functional>
#include <SDL.h>
#include "image.h"

//extern Image gImages[4];

enum BUTTON_EVENT_HANDLERS
{
    MOUSEBUTTONDOWN,
    MOUSEBUTTONUP,
    TOTAL_BUTTON_EVENT_HANDLERS
};


class Button: public Image
{
    public:
        Button();
        ~Button();

        bool handleEvent(SDL_Event* e);
        //void setOnClickHandler(void (*handler)(Button *));
        void setEventHandler(int, std::function<void(Button *)>);

    protected:
        //void (*eventHandlers[TOTAL_BUTTON_EVENT_HANDLERS])(Button *);
        std::function<void(Button *)> eventHandlers[TOTAL_BUTTON_EVENT_HANDLERS];
};

#endif
