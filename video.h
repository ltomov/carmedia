#ifndef VIDEO_H
#define VIDEO_H

#include <SDL.h>
#include <iostream>

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
}

using namespace std;

class Video
{
public:
    Video();
    ~Video();

    void render(SDL_Rect* clip = NULL, SDL_Rect* renderQuad = NULL);
    void setRenderer(SDL_Renderer *);
    int loadFromFile(string);
    void loadNextFrame();
    bool isLoaded();

protected:
	SDL_Renderer *mRenderer;
    SDL_Texture *mTexture;
    SDL_Texture *mTextures[330];    // TODO: unhardcode!

    void free();

    int position_x;
    int position_y;

    bool playingBackwards;
    bool _isLoaded;
    int currentFrameNumber;
    int totalFrames;

    AVFormatContext *pFormatCtx;
    AVCodecContext  *pCodecCtx;
    AVCodec         *pCodec;
    AVFrame         *pFrame;
    int             videoStream;

};

#endif
