#include "button.h"

#define CLICK_PADDING 5

Button::Button()
{
    Image();

    for (int i = 0; i < TOTAL_BUTTON_EVENT_HANDLERS; i++)
        eventHandlers[i] = NULL;
}

Button::~Button()
{
    for (int i = 0; i < TOTAL_BUTTON_EVENT_HANDLERS; i++)
        eventHandlers[i] = NULL;
}

bool Button::handleEvent( SDL_Event* e )
{
    //If mouse event happened
    if (e->type == SDL_MOUSEBUTTONDOWN || e->type == SDL_MOUSEBUTTONUP)
    {
        //Get mouse position
        int x, y;
        SDL_GetMouseState( &x, &y );
        //Check if mouse is in button
        bool inside = true;

        //Mouse is left of the button
        if( x < position_x - CLICK_PADDING )
        {
            inside = false;
        }
        //Mouse is right of the button
        else if( x > position_x + getWidth() + CLICK_PADDING )
        {
            inside = false;
        }
        //Mouse above the button
        else if( y < position_y - CLICK_PADDING)
        {
            inside = false;
        }
        //Mouse below the button
        else if( y > position_y + getHeight() + CLICK_PADDING)
        {
            inside = false;
        }

        //Mouse is inside button
        if (inside)
        {
            //Set mouse over sprite
            switch( e->type )
            {
                case SDL_MOUSEBUTTONDOWN:
                    //printf("inside\n");

                    // TODO: move this in some generic callback for all buttons?
                    this->setAlpha(50);

                    if (eventHandlers[MOUSEBUTTONDOWN] != NULL)
                        eventHandlers[MOUSEBUTTONDOWN](this);

                break;


                case SDL_MOUSEBUTTONUP:
                    //printf("inside up\n");

                    this->setAlpha(SDL_ALPHA_OPAQUE);

                    if (eventHandlers[MOUSEBUTTONUP] != NULL)
                        eventHandlers[MOUSEBUTTONUP](this);
                break;
            }

            // event handled
            return true;
        }

    }

    // event not handled
    return false;
}

void Button::setEventHandler(int event, std::function<void(Button *)> handler)
{
    eventHandlers[event] = handler;
}
