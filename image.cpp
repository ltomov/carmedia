#include <string>
#include <SDL.h>
#include <SDL_image.h>
#include "image.h"

Image::Image()
{
	mTexture = NULL;
	mRenderer = NULL;
	mWidth = 0;
	mHeight = 0;
    position_x = 0;
    position_y = 0;
}

Image::~Image()
{
	//Deallocate
	free();
}

bool Image::loadFromFile(std::string filename)
{
    //Get rid of preexisting texture
	free();

	//The final texture
	SDL_Texture* newTexture = NULL;

	//Load image at specified path
	SDL_Surface* loadedSurface = IMG_Load( filename.c_str() );
	if( loadedSurface == NULL )
	{
		printf( "Unable to load image %s! SDL_image Error: %s\n", filename.c_str(), IMG_GetError() );
	}
	else
	{
		//Color key image
		//SDL_SetColorKey( loadedSurface, SDL_TRUE, SDL_MapRGB( loadedSurface->format, 0, 0xFF, 0xFF ) );

		//Create texture from surface pixels
        newTexture = SDL_CreateTextureFromSurface( mRenderer, loadedSurface );
		if( newTexture == NULL )
		{
			printf( "Unable to create texture from %s! SDL Error: %s\n", filename.c_str(), SDL_GetError() );
		}
		else
		{
			//Get image dimensions
			mWidth = loadedSurface->w;
			mHeight = loadedSurface->h;
		}

		//Get rid of old loaded surface
		SDL_FreeSurface( loadedSurface );
	}


	//Return success
	mTexture = newTexture;

    // TODO: set for all?
    SDL_SetTextureBlendMode( mTexture, SDL_BLENDMODE_BLEND );


	return mTexture != NULL;
}

bool Image::loadFromText(std::string text, TTF_Font *font, SDL_Color *color, int wordWrapWidth, int wordWrapAtChar)
{
    if (mTexture != NULL)
        free();

    if (text.length() == 0)
        text = "[error]";

    textLabel = text;

    SDL_Surface *surf;
    if (wordWrapWidth > 0)
    {
        string newString;
        vector<string> arr = splitByLength(text, wordWrapAtChar);
        cout << arr.size() << endl;
        for (vector<string>::iterator arr_it = arr.begin(); arr_it != arr.end(); ++arr_it)
        {
            string str = *arr_it;
            cout << str << endl;
            if (str.find(' ') == string::npos)
                str += " ";

            newString += str;
        }

        surf = TTF_RenderUTF8_Blended_Wrapped(font, newString.c_str(), *color, wordWrapWidth);
    }
    else
        surf = TTF_RenderUTF8_Blended(font, text.c_str(), *color);

    mTexture = SDL_CreateTextureFromSurface(mRenderer, surf);

    mWidth = surf->w;
    mHeight = surf->h;

	//Clean up the surface and font
	SDL_FreeSurface(surf);

    return mTexture != NULL;
}

void Image::free()
{
	//Free texture if it exists
	if( mTexture != NULL )
	{
		SDL_DestroyTexture( mTexture );
		mTexture = NULL;
		mWidth = 0;
		mHeight = 0;
		position_x = 0;
		position_y = 0;
		textLabel = "";
	}
}


void Image::setAlpha(int alpha)
{
    SDL_SetTextureAlphaMod( mTexture, alpha );
}

void Image::setPosition(int x, int y)
{
    position_x = x;
    position_y = y;
}

void Image::render(SDL_Rect* clip, SDL_Rect* renderQuad)
{
	if (renderQuad == NULL)
	{
        SDL_Rect renderQuad2;

        if (clip == NULL)
        {
            renderQuad2.x = position_x;
            renderQuad2.y = position_y;
            renderQuad2.w = mWidth;
            renderQuad2.h = mHeight;

            renderQuad = &renderQuad2;
        }
        else
        {
            renderQuad->x = position_x + clip->x;
            renderQuad->y = position_y + clip->y;
            renderQuad->w = clip->w;
            renderQuad->h = clip->h;
        }
	}
	else
	{
        position_x = renderQuad->x;
        position_y = renderQuad->y;
        mWidth = renderQuad->w;
        mHeight = renderQuad->h;
	}


	//Render to screen
	SDL_RenderCopy(mRenderer, mTexture, clip, renderQuad);

}

void Image::setRenderer(SDL_Renderer *renderer)
{
    mRenderer = renderer;
}

int Image::getWidth()
{
	return mWidth;
}

int Image::getHeight()
{
	return mHeight;
}

int Image::getPositionX()
{
	return position_x;
}

int Image::getPositionY()
{
	return position_y;
}

string Image::getTextLabel()
{
    return textLabel;
}
