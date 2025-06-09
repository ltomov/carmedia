#include "video.h"

Video::Video()
{
    mRenderer = NULL;
    mTexture = NULL;
    position_x = 0;
    position_y = 0;
    pFormatCtx = NULL;
    pCodecCtx = NULL;
    pCodec = NULL;
    pFrame = NULL;
    videoStream = -1;
    playingBackwards = false;
    currentFrameNumber = 0;
    totalFrames = 0;
    _isLoaded = false;
}

Video::~Video()
{
    free();
}

void Video::free()
{
    if (mTexture != NULL)
    {
        // Free the YUV frame
        av_free(pFrame);
        // Close the codec
        avcodec_close(pCodecCtx);
        // Close the video file
        avformat_close_input(&pFormatCtx);

        SDL_DestroyTexture(mTexture);
        mTexture = NULL;
        //mRenderer = NULL;
        position_x = 0;
        position_y = 0;

        pFormatCtx = NULL;
        pCodecCtx = NULL;
        pCodec = NULL;
        pFrame = NULL;
        videoStream = -1;
        _isLoaded = false;

        for (int i = 0; i < 330; i++)
            SDL_DestroyTexture(mTextures[i]);


    }
}


int Video::loadFromFile(std::string fullPath)
{
    // Open video file
    if (avformat_open_input(&pFormatCtx, fullPath.c_str(), NULL, NULL)!=0)
    {
        fprintf(stderr, "Cannot open file %s!\n", fullPath.c_str());
        return -1;
    }


    // Retrieve stream information
    if (avformat_find_stream_info(pFormatCtx, NULL)<0)
    {
        fprintf(stderr, "Couldn't find stream information!\n");
        return -1; //
    }


    // Dump information about file onto standard error
    //av_dump_format(pFormatCtx, 0, fullPath.c_str(), 0);

    // Find the first video stream
    videoStream=-1;
    for (unsigned int i=0; i < pFormatCtx->nb_streams; i++)
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoStream=i;
            break;
        }

    if (videoStream==-1)
    {
        fprintf(stderr, "Didn't find a video stream\n");
        return -1;
    }


    // Get a pointer to the codec context for the video stream
    pCodecCtx = pFormatCtx->streams[videoStream]->codec;

    // Find the decoder for the video stream
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (pCodec == NULL)
    {
        fprintf(stderr, "Unsupported codec!\n");
        return -1; // Codec not found
    }

    // TODO: what's that?
    AVDictionary *optionsDict = NULL;

    // Open codec
    if (avcodec_open2(pCodecCtx, pCodec, &optionsDict) < 0)
        return -1; // Could not open codec

    // Allocate video frame
    pFrame = av_frame_alloc();

    mTexture = SDL_CreateTexture(mRenderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STATIC, pCodecCtx->width, pCodecCtx->height);
    for (int i = 0; i < 330; i++)
        mTextures[i] = SDL_CreateTexture(mRenderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STATIC, pCodecCtx->width, pCodecCtx->height);

    totalFrames = pFormatCtx->streams[videoStream]->nb_frames;
    //printf("Total frames: %d\n", (int)pFormatCtx->streams[videoStream]->nb_frames);


    // TODO: support both loading each frame from disk and preloading them in RAM
    int frameFinished;

    AVPacket packet;
    int i = 0;

    while (av_read_frame(pFormatCtx, &packet) >= 0 && i < totalFrames)
    {
        // Is this a packet from the video stream?
        if (packet.stream_index == videoStream)
        {
            // Decode video frame
            avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);

            if (frameFinished)
            {
                SDL_UpdateYUVTexture(
                    mTextures[i++], NULL, pFrame->data[0], pFrame->linesize[0],
                    pFrame->data[1], pFrame->linesize[1],
                    pFrame->data[2], pFrame->linesize[2]
                );
            }
        }
    }

    av_free_packet(&packet);

    _isLoaded = true;

    return 0;
}

void Video::loadNextFrame()
{
    if (!_isLoaded)
        return;

    if (playingBackwards)
    {
        currentFrameNumber--;

        if (currentFrameNumber == 0)
        {
            playingBackwards = false;
        }
    }
    else
    {
        currentFrameNumber++;

        if (currentFrameNumber >= totalFrames - 4)
        {
            playingBackwards = true;
            currentFrameNumber --;
        }
    }

    mTexture = mTextures[currentFrameNumber];
}

void Video::render(SDL_Rect* clip, SDL_Rect* renderQuad)
{
    if (!_isLoaded)
        return;

	if (renderQuad == NULL)
	{
        SDL_Rect renderQuad2;

        if (clip == NULL)
        {
            renderQuad2.x = position_x;
            renderQuad2.y = position_y;
            renderQuad2.w = pCodecCtx->width;
            renderQuad2.h = pCodecCtx->height;

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

	//Render to screen
	SDL_RenderCopy(mRenderer, mTexture, clip, renderQuad);
}

void Video::setRenderer(SDL_Renderer *renderer)
{
    mRenderer = renderer;
}

bool Video::isLoaded()
{
    return _isLoaded;
}
