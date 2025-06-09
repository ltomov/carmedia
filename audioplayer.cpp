#include "audioplayer.h"
#include <iomanip>

#include <fileref.h>
#include <tag.h>
#include <tpropertymap.h>

// TODO: make singleton

AudioPlayer::AudioPlayer()
{
    playerMode = AudioPlayer::PLAYER_MODE_LOCAL;
    playerStatus = AudioPlayer::PLAYER_STATUS_STOPPED;
    currentTrack = 0;

    mMusic = NULL;

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
    {
        printf( "SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError() );
    }
}
AudioPlayer::~AudioPlayer()
{
    Mix_FreeMusic(mMusic);
    mMusic = NULL;

    Mix_Quit();
}

void AudioPlayer::loadTracks(string fullPaths[], int cntFiles)
{
    trackPaths.clear();

    for (int i = 0; i < cntFiles; i++)
        trackPaths.push_back(fullPaths[i]);
}


void AudioPlayer::play()
{
    playerStatus = AudioPlayer::PLAYER_STATUS_PLAYING;

    if (getPlayerMode() == AudioPlayer::PLAYER_MODE_BLUETOOTH)
        play_bluetooth();
    else
        play_local();
}

void AudioPlayer::pause()
{
    playerStatus = AudioPlayer::PLAYER_STATUS_PAUSED;

    if (getPlayerMode() == AudioPlayer::PLAYER_MODE_BLUETOOTH)
        pause_bluetooth();
    else
        pause_local();
}

void AudioPlayer::resume()
{
    playerStatus = AudioPlayer::PLAYER_STATUS_PLAYING;

    if (getPlayerMode() == AudioPlayer::PLAYER_MODE_BLUETOOTH)
        resume_bluetooth();
    else
        resume_local();
}

void AudioPlayer::playNextTrack()
{
    if (getPlayerMode() == AudioPlayer::PLAYER_MODE_BLUETOOTH)
        playNextTrack_bluetooth();
    else
        playNextTrack_local();
}

void AudioPlayer::playPreviousTrack()
{
    if (getPlayerMode() == AudioPlayer::PLAYER_MODE_BLUETOOTH)
        playPreviousTrack_bluetooth();
    else
        playPreviousTrack_local();
}




void AudioPlayer::play_bluetooth()
{
    PhoneLib::instance()->getMediaPlayer()->Play();

    getTrackInfo_bluetooth();
}

void AudioPlayer::pause_bluetooth()
{
    PhoneLib::instance()->getMediaPlayer()->Pause();
}

void AudioPlayer::resume_bluetooth()
{
    PhoneLib::instance()->getMediaPlayer()->Play();

    getTrackInfo_bluetooth();
}

void AudioPlayer::playNextTrack_bluetooth()
{
    PhoneLib::instance()->getMediaPlayer()->Next();

    usleep(600000);
    getTrackInfo_bluetooth();
}

void AudioPlayer::playPreviousTrack_bluetooth()
{
    PhoneLib::instance()->getMediaPlayer()->Previous();

    usleep(600000);
    getTrackInfo_bluetooth();
}

void AudioPlayer::getTrackInfo_bluetooth()
{
    map<string, ::DBus::Variant> ti = PhoneLib::instance()->getMediaPlayer()->Track();

    string artist = ti["Artist"];
    string title = ti["Title"];
    string album = ti["Album"];
    string genre = ti["Genre"];
    uint32_t duration = ti["Duration"];

    mCurrentTrackInfo.artist = artist;
    mCurrentTrackInfo.title = title;
    mCurrentTrackInfo.album = album;
    mCurrentTrackInfo.genre = genre;
    mCurrentTrackInfo.lengthSeconds = duration % 60;
    mCurrentTrackInfo.lengthMinutes = (duration - mCurrentTrackInfo.lengthSeconds) / 60;
}




void AudioPlayer::play_local()
{
    if (mMusic != NULL)
        Mix_FreeMusic(mMusic);

    mMusic = Mix_LoadMUS( trackPaths[currentTrack].c_str() );
    if (mMusic == NULL)
    {
        printf("Failed to load music %s! SDL_mixer Error: %s\n", trackPaths[currentTrack].c_str(), Mix_GetError() );
    }
    else
    {
        Mix_PlayMusic(mMusic, 1);

        printf("Playing: %s\n", trackPaths[currentTrack].c_str() );
    }

    getTrackInfo_local(trackPaths[currentTrack]);
}

void AudioPlayer::getTrackInfo_local(string filePath)
{
    TagLib::FileRef f(filePath.c_str());

    if(!f.isNull() && f.tag())
    {
        TagLib::Tag *tag = f.tag();

        cout << "-- TAG (basic) --" << endl;
        cout << "title   - \"" << tag->title()   << "\"" << endl;
        cout << "artist  - \"" << tag->artist()  << "\"" << endl;
        cout << "album   - \"" << tag->album()   << "\"" << endl;
        cout << "year    - \"" << tag->year()    << "\"" << endl;
        cout << "comment - \"" << tag->comment() << "\"" << endl;
        cout << "track   - \"" << tag->track()   << "\"" << endl;
        cout << "genre   - \"" << tag->genre()   << "\"" << endl;

        TagLib::PropertyMap tags = f.file()->properties();

        unsigned int longest = 0;
            for(TagLib::PropertyMap::ConstIterator i = tags.begin(); i != tags.end(); ++i) {
            if (i->first.size() > longest) {
              longest = i->first.size();
            }
        }

        cout << "-- TAG (properties) --" << endl;
        for(TagLib::PropertyMap::ConstIterator i = tags.begin(); i != tags.end(); ++i) {
            for(TagLib::StringList::ConstIterator j = i->second.begin(); j != i->second.end(); ++j) {
              cout << left << std::setw(longest) << i->first << " - " << '"' << *j << '"' << endl;
            }
        }

        TagLib::AudioProperties *properties = f.audioProperties();

        int lengthSeconds = properties->length() % 60;
        int lengthMinutes = (properties->length() - lengthSeconds) / 60;

        cout << "-- AUDIO --" << endl;
        cout << "bitrate     - " << properties->bitrate() << endl;
        cout << "sample rate - " << properties->sampleRate() << endl;
        cout << "channels    - " << properties->channels() << endl;
        cout << "length      - " << lengthMinutes << ":" << setfill('0') << setw(2) << lengthSeconds << endl;



        mCurrentTrackInfo.title = tag->title().to8Bit(true);
        mCurrentTrackInfo.artist = tag->artist().to8Bit(true);
        mCurrentTrackInfo.album = tag->album().to8Bit(true);
        mCurrentTrackInfo.genre = tag->genre().to8Bit(true);
        mCurrentTrackInfo.lengthMinutes = lengthMinutes;
        mCurrentTrackInfo.lengthSeconds = lengthSeconds;
        mCurrentTrackInfo.bitrate = properties->bitrate();
    }
}

trackInfo_t *AudioPlayer::getCurrentTrackInfo()
{
    return &mCurrentTrackInfo;
}

void AudioPlayer::pause_local()
{
    Mix_PauseMusic();
}

void AudioPlayer::resume_local()
{
    Mix_ResumeMusic();
}

void AudioPlayer::playNextTrack_local()
{
    currentTrack++;
    if (currentTrack >= (int)trackPaths.size())
        currentTrack = 0;

    play();
}

void AudioPlayer::playPreviousTrack_local()
{
    currentTrack--;
    if (currentTrack < 0)
        currentTrack = 0;

    play();
}

unsigned char AudioPlayer::getPlayerMode()
{
    return playerMode;
}

unsigned char AudioPlayer::getPlayerStatus()
{
    return playerStatus;
}
