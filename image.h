#ifndef IMAGE_H
#define IMAGE_H

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <string>
#include <vector>
#include "utils.h"

using namespace std;

class Image
{
public:
    //Initializes internal variables
    Image();

    ~Image();


    bool loadFromFile(string);
    bool loadFromText(string, TTF_Font *, SDL_Color *, int wordWrapWidth = 0, int wordWrapAtChar = 0);

    //Renders texture at given point
    void render(SDL_Rect* clip = NULL, SDL_Rect* renderQuad = NULL);

    void setPosition(int x, int y);
    void setRenderer(SDL_Renderer *);
    void setAlpha(int);

    //Gets image dimensions
    int getWidth();
    int getHeight();

    int getPositionX();
    int getPositionY();

    string getTextLabel();


protected:
    SDL_Texture *mTexture;
    SDL_Renderer *mRenderer;

    //Deallocates texture
    void free();

    //Image dimensions
    int mWidth;
    int mHeight;

    int position_x;
    int position_y;

    string textLabel;
};

#endif
