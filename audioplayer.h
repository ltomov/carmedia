#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include <dirent.h>
#include <SDL_mixer.h>
#include <iostream>
#include <vector>
#include <unistd.h>

#include "phonelib/phonelib.h"

using namespace std;

struct trackInfo_t
{
    string artist;
    string title;
    string album;
    string genre;
    int lengthMinutes;
    int lengthSeconds;
    int bitrate;
};

class AudioPlayer
{
public:

    static const uint8_t PLAYER_MODE_LOCAL = 0;
    static const uint8_t PLAYER_MODE_BLUETOOTH = 1;

    static const uint8_t PLAYER_STATUS_PLAYING = 0;
    static const uint8_t PLAYER_STATUS_PAUSED = 1;
    static const uint8_t PLAYER_STATUS_STOPPED = 2;

    AudioPlayer();
    ~AudioPlayer();

    void loadTracks(string [], int);


    void play();
    void pause();
    void resume();
    void playNextTrack();
    void playPreviousTrack();

    void play_local();
    void pause_local();
    void resume_local();
    void playNextTrack_local();
    void playPreviousTrack_local();

    void play_bluetooth();
    void pause_bluetooth();
    void resume_bluetooth();
    void playNextTrack_bluetooth();
    void playPreviousTrack_bluetooth();


    void setPlayerMode(unsigned char);
    unsigned char getPlayerMode();
    unsigned char getPlayerStatus();
    trackInfo_t *getCurrentTrackInfo();

protected:
    void getTrackInfo_bluetooth();
    void getTrackInfo_local(string);

    uint8_t playerMode;
    uint8_t playerStatus;
    Mix_Music *mMusic;
    trackInfo_t mCurrentTrackInfo;

    vector<string> trackPaths;
    int currentTrack;
};

#endif

