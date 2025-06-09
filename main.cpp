#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <SDL_ttf.h>
#include <termios.h>
#include <iostream>
#include <sqlite3.h>

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
}

#include "settings.h"
#include "button.h"
#include "image.h"
#include "phonelib/phonelib.h"
#include "wt32i.h"
#include "common.h"
#include "touch.h"
#include "audioplayer.h"
#include "video.h"
#include "animation.h"
#include "filebrowser.h"


using namespace std;



bool init();
bool loadMedia();
void close();
bool loadContactsFromDB();
int threadGetPlayStatus(void *);
int threadGetTrackInfo(void *);
int threadWT32WriteRaw(void *);
int threadWT32GetCallHistory(void *);
int threadBluetoothConnect(void *);
Uint32 timer_1sCallback(Uint32 interval, void* param);
Uint32 timer_10sCallback(Uint32 interval, void* param);
Uint32 timer_33fpsCallback(Uint32 interval, void* param);

static int sqliteLoadSettings(void *, int, char **, char **);

enum IMAGES
{
    IMG_BACKGROUND,
    IMG_BACKGROUND2,
    IMG_BUTTON_H_SEPARATOR,
    IMG_BUTTON_SELECTED,
    IMG_TOP_LINE,
    BGR_WHITE_GREEN,
    //IMG_MAIN_MENU_BACKGROUND,
    IMG_MAIN_MENU_SEPARATOR,
    IMG_ARROW_RIGHT,
    TOTAL_IMAGES
};

enum TOP_ICONS
{
    //ICO_PLAYER,
    //ICO_CONTACTS,
    //ICO_MESSAGES,
    //ICO_CALLS,
    //ICO_BTN_BT_SYNC,
    ICO_MAIN_MENU,
    TOTAL_TOP_ICONS
};

enum PLAYER_BUTTONS
{
    BTN_PLAYER_SHUFFLE,
    BTN_PLAYER_REPEAT,
    BTN_PLAYER_PREVIOUS,
    BTN_PLAYER_NEXT,
    BTN_PLAYER_PLAY,
    //PLAYER_POSITION_BACKGROUND,
    PLAYER_POSITION_DOT,
    TOTAL_PLAYER_BUTTONS
};

enum PAGES
{
    PAGE_DASHBOARD,
    PAGE_PLAYER,
    PAGE_MESSAGES,
    PAGE_CONTACTS,
    PAGE_CALLS,
    PAGE_SETTINGS,
    PAGE_FILEBROWSER
};

enum OVERLAYS
{
    OVERLAY_MAIN_MANU,
    TOTAL_OVERLAYS
};

enum MAIN_MENU_BUTTONS
{
    MAIN_MENU_BUTTON_PLAYER,
    MAIN_MENU_BUTTON_MESSAGES,
    MAIN_MENU_BUTTON_CONTACTS,
    MAIN_MENU_BUTTON_CALLS,
    MAIN_MENU_BUTTON_SETTINGS,
    MAIN_MENU_BUTTON_BT_SYNC,
    MAIN_MENU_BUTTON_FILEBROWSER,
    TOTAL_MAIN_MENU_BUTTONS
};

// TODO: for titles use only one object - just change text
enum LABELS
{
    LABEL_TITLE,
    LABEL_PLAYER_TRACK_NAME,
    LABEL_PLAYER_TRACK_NAME2,
    LABEL_PLAYER_ARTIST,
    LABEL_PLAYER_PLAYING_TIME,
    LABEL_FPS,
    LABEL_HISTORY_CONTACT_NAME,
    TOTAL_LABELS
};

enum SETTING_BACKGROUNDS
{
    BGR_1,
    BGR_2,
    BGR_3,
    BGR_4,
    BGR_5,
    TOTAL_SETTING_BACKGROUNDS
};

int gCurrentPage = PAGE_DASHBOARD;


//The font that's going to be used
TTF_Font *fontLato = NULL;
TTF_Font *fontArial = NULL;

SDL_Color colorWhite = { 255, 255, 255, SDL_ALPHA_OPAQUE }, colorGrey = { 80, 80, 80, SDL_ALPHA_OPAQUE }, colorBlack = { 0, 0, 0, SDL_ALPHA_OPAQUE };


Button gTopIcons[TOTAL_TOP_ICONS];
Button gPlayerButtons[TOTAL_PLAYER_BUTTONS];
Button gMainMenuButtons[TOTAL_MAIN_MENU_BUTTONS];
Button gButtonsSettingBackgrounds[TOTAL_SETTING_BACKGROUNDS];

Image gImages[TOTAL_IMAGES];
Image gLabels[TOTAL_LABELS];

bool gVisibleOverlays[TOTAL_OVERLAYS];

AudioPlayer gAudioPlayer;
Animation gAnimations;

Touch *Touch::m_instance = 0;
PhoneLib *PhoneLib::m_instance = 0;
FileBrowser *FileBrowser::m_instance = 0;

SDL_Window* gWindow = NULL;
SDL_Renderer* gRenderer = NULL;

int gMainMenuBgrWidth = 0;

WT32i wt32i;
//WT32CallHistory_t gCallHistory[30];

vector<CallHistory_t> gCallHistory;
vector<VCard_t> gContacts;

// TODO: crashes when gtkterm is connected to wt32i

/*
    TODO:
    Even after the procedure started in the thread returns, there still exist some resources allocated to the thread.
    To free these resources, use SDL_WaitThread to wait for the thread to finish and obtain the status code of the thread.
    If not done so, SDL_CreateThread will hang after about 1010 successfully created threads (tested on GNU/Linux).
*/

bool gQuit = false;

// TODO: user some playerTrackInfo struct
string gArtistName = "Artist", gTrackName = "Track name";
int gPlayerTrackTotalPlayingMinutes = 0, gPlayerTrackTotalPlayingSeconds = 0,
    gPlayerTrackCurrentPlayingMinutes = 0, gPlayerTrackCurrentPlayingSeconds = 0;

int gFPS = 0, gFPSCounter = 0;

int gBackgroundAlpha1 = SDL_ALPHA_OPAQUE;
int gBackgroundAlpha2 = SDL_ALPHA_TRANSPARENT;
int gAlphaFading = 0;

sqlite3 *gDB;

Video gVideo;

// TODO: remove in production
bool gIsBoard = false;


DBus::BusDispatcher dispatcher;

int main(int argc, char *argv[])
{
    if (argc > 1 && strcmp(argv[1], "board") == 0)
    {
        gIsBoard = true;
    }


    DBus::default_dispatcher = &dispatcher;

    DBus::Connection dbusSystemBus = DBus::Connection::SystemBus();
    DBus::Connection dbusSessionBus = DBus::Connection::SessionBus();

    PhoneLib::instance()->init(&dbusSystemBus, &dbusSessionBus);

    // TODO: do contacts really need to be stored in DB??
    sqlite3_open("db", &gDB);

    init();
    loadMedia();
    // TODO: remove from here
    //wt32i.writeRaw("call cc:fa:00:10:81:8e 112f pbap");

    gVideo.setRenderer(gRenderer);

    //gVideo.loadFromFile("/media/lacho3/Data/WEB/Envato Downloads/Mental_Freedom/00164_A_1.mov");
    //gVideo.loadFromFile("/media/lacho3/Data/WEB/Envato Downloads/Mental_Freedom/00165_AA.mov");
    //gVideo.loadFromFile("/media/lacho3/Data/WEB/Envato Downloads/Mental_Freedom/00167.mov");

/*
    auto fnLoadVideo = [](void *data) -> int {
        gVideo.loadFromFile("/media/lacho3/Data/WEB/Envato Downloads/Mental_Freedom/00168_B.mov");
        return 0;
    };
    SDL_CreateThread(fnLoadVideo, NULL, NULL);
*/
    //gVideo.loadFromFile("/media/lacho3/Data/WEB/Envato Downloads/Mental_Freedom/00168_B.mov");

    //gVideo.loadFromFile("/media/lacho3/Data/WEB/Envato Downloads/beach.mp4");
    //gVideo.loadFromFile("/media/lacho3/Data/WEB/Envato Downloads/coconut.mp4");
    //gVideo.loadFromFile("/media/lacho3/Data/WEB/Envato Downloads/truck.mp4");

    //SDL_Rect videoRenderQuad = {0, 0, 1920, 1080};
    SDL_Rect videoRenderQuad = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};



    //SDL_CreateThread(threadGetTrackInfo, NULL, NULL); // TODO: remove these functions?
    //SDL_CreateThread(threadGetPlayStatus, NULL, NULL);
    SDL_CreateThread(threadBluetoothConnect, NULL, NULL);

    SDL_Event e;

    // TODO: kill on exit?
    SDL_AddTimer(1000, timer_1sCallback, NULL);
    //SDL_AddTimer(10 * 1000, timer_10sCallback, NULL); // TODO: delete those functions?
    SDL_AddTimer(33, timer_33fpsCallback, NULL);


    //While application is running
    while( !gQuit )
    {
        //Handle events on queue
        while( SDL_PollEvent( &e ) != 0 )
        {
            bool eventHandled = false;

            //User requests quit
            if( e.type == SDL_QUIT )
            {
                gQuit = true;
            }
            else if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP)
            {
                for (int i = 0; i < TOTAL_TOP_ICONS; i++)
                {
                    eventHandled = gTopIcons[i].handleEvent(&e);
                    if (eventHandled)
                        break;
                }

                if (!eventHandled && gCurrentPage == PAGE_PLAYER)
                {
                    for (int i = 0; i < TOTAL_PLAYER_BUTTONS; i++)
                    {
                        eventHandled = gPlayerButtons[i].handleEvent(&e);
                        if (eventHandled)
                            break;
                    }
                }

                if (!eventHandled && gVisibleOverlays[OVERLAY_MAIN_MANU] == true)
                {
                    for (int i = 0; i < TOTAL_MAIN_MENU_BUTTONS; i++)
                    {
                        eventHandled = gMainMenuButtons[i].handleEvent(&e);
                        if (eventHandled)
                            break;
                    }
                }


                if (!eventHandled && gCurrentPage == PAGE_SETTINGS)
                {
                    for (int i = 0; i < TOTAL_SETTING_BACKGROUNDS; i++)
                    {
                        eventHandled = gButtonsSettingBackgrounds[i].handleEvent(&e);
                        if (eventHandled)
                            break;
                    }
                }


/*
                if (!eventHandled && e.type == SDL_MOUSEBUTTONDOWN)
                {
                    gDragPrevYPos = gImages[BGR_WHITE_GREEN].getPositionY() - gCallHistory.size() * 80 + 94;
                    gDragPrevXPos = gImages[BGR_WHITE_GREEN].getPositionX() - ((gContacts.size() + 1) / CNT_CONTACTS_PER_ROW) * (gImages[BGR_WHITE_GREEN].getWidth() + 100) + 220 - Touch::instance()->getInertia();    // TODO: dafuq with ->getInertia() ?
                }
*/
            }

            if (gCurrentPage == PAGE_FILEBROWSER && !Touch::instance()->isScreenMoving())
            {
                FileBrowser::instance()->handleEvents(&e);
            }


            // TODO: dafuq be? purvo handle na touch-a, predi butonite? i bez !eventHandled?
            if (!eventHandled)
            {
                Touch::instance()->handleEvent(&e);
            }

        }




        if (gAlphaFading)
        {
            gBackgroundAlpha1 ++;
            gBackgroundAlpha2 --;

            if (gBackgroundAlpha2 == SDL_ALPHA_TRANSPARENT)
                gAlphaFading = 0;
        }
        else
        {
            gBackgroundAlpha1 --;
            gBackgroundAlpha2 ++;

            if (gBackgroundAlpha1 == SDL_ALPHA_TRANSPARENT)
                gAlphaFading = 1;
        }

        SDL_RenderClear(gRenderer);

        //gImages[IMG_BACKGROUND2].setAlpha(gBackgroundAlpha2);
        //gImages[IMG_BACKGROUND2].render();


        //gImages[IMG_BACKGROUND].setAlpha(gBackgroundAlpha1);
        gImages[IMG_BACKGROUND].render();

        //if (gFPSCounter % 2 == 0)
          //  gVideo.loadNextFrame();

        //gVideo.render(NULL, &videoRenderQuad);


        gImages[IMG_TOP_LINE].render();

/*
        gImages[IMG_BUTTON_H_SEPARATOR].setPosition(
            (SCREEN_WIDTH / 2) - gTopIcons[ICO_MESSAGES].getWidth() / 2 - 30,
            160 - gImages[IMG_BUTTON_H_SEPARATOR].getHeight() / 2);
        gImages[IMG_BUTTON_H_SEPARATOR].render();

        gImages[IMG_BUTTON_H_SEPARATOR].setPosition(
            (SCREEN_WIDTH / 2) + (gTopIcons[ICO_MESSAGES].getWidth() / 2) + 30,
            160 - gImages[IMG_BUTTON_H_SEPARATOR].getHeight() / 2);
        gImages[IMG_BUTTON_H_SEPARATOR].render();



        gImages[IMG_BUTTON_H_SEPARATOR].setPosition(
            gTopIcons[ICO_CONTACTS].getPositionX() + gTopIcons[ICO_CONTACTS].getWidth() + 20,
            160 - gImages[IMG_BUTTON_H_SEPARATOR].getHeight() / 2);
        gImages[IMG_BUTTON_H_SEPARATOR].render();
*/





        if (gCurrentPage == PAGE_DASHBOARD)
        {

            gLabels[LABEL_TITLE].loadFromText("Dashboard", fontLato, &colorWhite);

        }
        else if (gCurrentPage == PAGE_PLAYER)
        {
            gLabels[LABEL_TITLE].loadFromText("Player", fontLato, &colorWhite);


            gPlayerButtons[BTN_PLAYER_SHUFFLE].render();
            gPlayerButtons[BTN_PLAYER_REPEAT].render();
            gPlayerButtons[BTN_PLAYER_PREVIOUS].render();
            gPlayerButtons[BTN_PLAYER_PLAY].render();
            gPlayerButtons[BTN_PLAYER_NEXT].render();


            gLabels[LABEL_PLAYER_TRACK_NAME2].loadFromText(gTrackName, fontArial, &colorBlack);
            gLabels[LABEL_PLAYER_TRACK_NAME2].setPosition(gLabels[LABEL_PLAYER_TRACK_NAME].getPositionX() + 1, gLabels[LABEL_PLAYER_TRACK_NAME].getPositionY() - 1);
            gLabels[LABEL_PLAYER_TRACK_NAME2].render();
            gLabels[LABEL_PLAYER_TRACK_NAME].loadFromText(gTrackName, fontArial, &colorWhite);
            gLabels[LABEL_PLAYER_TRACK_NAME].setPosition(SCREEN_WIDTH / 2 - gLabels[LABEL_PLAYER_TRACK_NAME].getWidth() / 2, 300);
            gLabels[LABEL_PLAYER_TRACK_NAME].render();

            gLabels[LABEL_PLAYER_ARTIST].loadFromText(gArtistName, fontArial, &colorWhite);
            gLabels[LABEL_PLAYER_ARTIST].setPosition(SCREEN_WIDTH / 2 - gLabels[LABEL_PLAYER_ARTIST].getWidth() / 2, 330);
            gLabels[LABEL_PLAYER_ARTIST].render();


            char buf1[3], buf2[2], buf3[3], buf4[4];
            sprintf(buf1, "%d", gPlayerTrackTotalPlayingMinutes);
            sprintf(buf2, "%02d", gPlayerTrackTotalPlayingSeconds);
            sprintf(buf3, "%d", gPlayerTrackCurrentPlayingMinutes);
            sprintf(buf4, "%02d", gPlayerTrackCurrentPlayingSeconds);

            gLabels[LABEL_PLAYER_PLAYING_TIME].loadFromText(string(buf3) + ":" + string(buf4) + " / " + string(buf1) + ":" + string(buf2), fontArial, &colorWhite);
            gLabels[LABEL_PLAYER_PLAYING_TIME].setPosition(SCREEN_WIDTH - 200 - gLabels[LABEL_PLAYER_PLAYING_TIME].getWidth(), SCREEN_HEIGHT - PLAYER_BUTTONS_BOTTOM_SPACING - gLabels[LABEL_PLAYER_PLAYING_TIME].getHeight() / 2);
            gLabels[LABEL_PLAYER_PLAYING_TIME].render();

            //gPlayerButtons[PLAYER_POSITION_BACKGROUND].render();

            // TODO: use global object
            SDL_Rect fillRect = { 0, gPlayerButtons[BTN_PLAYER_PLAY].getPositionY() - 100 - 6, SCREEN_WIDTH, 12 };
            SDL_SetRenderDrawColor( gRenderer, 0x22, 0x22, 0x33, 0xaa );
            SDL_RenderFillRect( gRenderer, &fillRect );

            int playerPositionDot_x = 0 - gPlayerButtons[PLAYER_POSITION_DOT].getWidth() + 40;
            // +1 to avoid division by zero
            int pixelsPerSecond = SCREEN_WIDTH / (gPlayerTrackTotalPlayingMinutes * 60 + gPlayerTrackTotalPlayingSeconds + 1);  // TODO: when seconds are more than width, this will output 0!
            playerPositionDot_x = playerPositionDot_x + (pixelsPerSecond * (gPlayerTrackCurrentPlayingMinutes * 60 + gPlayerTrackCurrentPlayingSeconds));

            gPlayerButtons[PLAYER_POSITION_DOT].setPosition(playerPositionDot_x, gPlayerButtons[BTN_PLAYER_PLAY].getPositionY() - 100 - gPlayerButtons[PLAYER_POSITION_DOT].getHeight() / 2);
            gPlayerButtons[PLAYER_POSITION_DOT].render();
        }
        else if (gCurrentPage == PAGE_MESSAGES)
        {
            gLabels[LABEL_TITLE].loadFromText("Messages", fontLato, &colorWhite);
        }
        else if (gCurrentPage == PAGE_CONTACTS)
        {
            gLabels[LABEL_TITLE].loadFromText("Contacts", fontLato, &colorWhite);


            unsigned int i = 0;
            int col = -1;
            int row = -1;
            for (i = 0; i < gContacts.size(); i++)
            {
                /*
                int contactPositionY = i * 80 + Touch::instance()->getDragVerticalLength();
                gImages[BGR_WHITE_GREEN].setPosition(SCREEN_WIDTH / 2 - gImages[BGR_WHITE_GREEN].getWidth() / 2, contactPositionY - gLabels[LABEL_HISTORY_CONTACT_NAME].getHeight() / 2);

                if (contactPositionY > -gImages[BGR_WHITE_GREEN].getHeight() && contactPositionY < SCREEN_HEIGHT + 30)
                {
                    gImages[BGR_WHITE_GREEN].render();

                    gLabels[LABEL_HISTORY_CONTACT_NAME].loadFromText(gContacts[i].name, fontArial, &colorGrey);
                    gLabels[LABEL_HISTORY_CONTACT_NAME].setPosition(gImages[BGR_WHITE_GREEN].getPositionX() + 80, contactPositionY);
                    gLabels[LABEL_HISTORY_CONTACT_NAME].render();
                }
                */

                row++;
                if (i % CONTACTS_CNT_ITEMS_PER_ROW == 0)
                {
                    col++;
                    row = 0;
                }

                int contactPositionX = 200 + (col * (CONTACTS_BUTTON_WIDTH + CONTACTS_BUTTON_HSPACING)) + Touch::instance()->getDragScreenOffsetX();

                int contactPositionY = 250 + row * (CONTACTS_BUTTON_HEIGHT + CONTACTS_BUTTON_VSPACING);
                gImages[BGR_WHITE_GREEN].setPosition(contactPositionX, contactPositionY);




                // render if contact is inside visible area
                if (contactPositionX > 0 - gImages[BGR_WHITE_GREEN].getWidth() && contactPositionX < SCREEN_WIDTH)
                {
                    /*SDL_Rect clip1 = {0, 0, gImages[BGR_WHITE_GREEN].getWidth(), gImages[BGR_WHITE_GREEN].getHeight()};
                    if (gImages[BGR_WHITE_GREEN].getPositionX() < 130)
                    {
                        clip1.x = 130 - gImages[BGR_WHITE_GREEN].getPositionX();
                        clip1.w = gImages[BGR_WHITE_GREEN].getWidth() - clip1.x;
                    }
                    if (gImages[BGR_WHITE_GREEN].getPositionX() < -50)
                        gImages[BGR_WHITE_GREEN].setAlpha(SDL_ALPHA_OPAQUE + (gImages[BGR_WHITE_GREEN].getPositionX() + 50) * 1.84);
                    else
                        gImages[BGR_WHITE_GREEN].setAlpha(SDL_ALPHA_OPAQUE);

                    gImages[BGR_WHITE_GREEN].render(&clip1);
*/

/*
                    if (gImages[BGR_WHITE_GREEN].getPositionX() < -200)
                        gImages[BGR_WHITE_GREEN].setAlpha(SDL_ALPHA_OPAQUE + (gImages[BGR_WHITE_GREEN].getPositionX() + 200) * 2.16);
                    else if (gImages[BGR_WHITE_GREEN].getPositionX() > SCREEN_WIDTH - 200)
                        gImages[BGR_WHITE_GREEN].setAlpha((SCREEN_WIDTH - gImages[BGR_WHITE_GREEN].getPositionX() - 200) * 1.29);
                    else
                        gImages[BGR_WHITE_GREEN].setAlpha(SDL_ALPHA_OPAQUE);
*/

                    // TODO: this is INSANELY slow (event when rect is a global var). Use a transparent png instead
                    SDL_Rect contactsBackgroundRect = { contactPositionX, contactPositionY, CONTACTS_BUTTON_WIDTH, CONTACTS_BUTTON_HEIGHT };
                    int alpha;
                    if (contactPositionX < -200)
                        alpha = (contactPositionX + 200) * 2.16;
                    else if (contactPositionX > SCREEN_WIDTH - 200)
                        alpha = ((SCREEN_WIDTH - contactPositionX - 200) * 1.29);
                    else
                        alpha = 0x99;

                    SDL_SetRenderDrawColor( gRenderer, 0x22, 0x22, 0x44, alpha );
                    SDL_RenderFillRect( gRenderer, &contactsBackgroundRect );

                    //gImages[BGR_WHITE_GREEN].render();


                    gLabels[LABEL_HISTORY_CONTACT_NAME].loadFromText(gContacts[i].name, fontArial, &colorWhite);
                    gLabels[LABEL_HISTORY_CONTACT_NAME].setPosition(contactPositionX + 80, contactPositionY + 20);

                    if (gLabels[LABEL_HISTORY_CONTACT_NAME].getPositionX() < -200)
                        gLabels[LABEL_HISTORY_CONTACT_NAME].setAlpha(SDL_ALPHA_OPAQUE + (gLabels[LABEL_HISTORY_CONTACT_NAME].getPositionX() + 200) * 4);
                    else
                        gLabels[LABEL_HISTORY_CONTACT_NAME].setAlpha(SDL_ALPHA_OPAQUE);


                    gLabels[LABEL_HISTORY_CONTACT_NAME].render();
                }

            }
        }
        else if (gCurrentPage == PAGE_CALLS)
        {
            gLabels[LABEL_TITLE].loadFromText("Calls", fontLato, &colorWhite);

            unsigned int i = 0;
            for (i = 0; i < gCallHistory.size(); i++)
            {
                int contactPositionY = i * 80 + Touch::instance()->getDragScreenOffsetY();

                gImages[BGR_WHITE_GREEN].setPosition(SCREEN_WIDTH / 2 - gImages[BGR_WHITE_GREEN].getWidth() / 2, contactPositionY - gLabels[LABEL_HISTORY_CONTACT_NAME].getHeight() / 2);

                if (contactPositionY > -gImages[BGR_WHITE_GREEN].getHeight() && contactPositionY < SCREEN_HEIGHT + 30)
                {
                    gImages[BGR_WHITE_GREEN].render();

                    gLabels[LABEL_HISTORY_CONTACT_NAME].loadFromText(gCallHistory[i].name, fontArial, &colorGrey);
                    gLabels[LABEL_HISTORY_CONTACT_NAME].setPosition(gImages[BGR_WHITE_GREEN].getPositionX() + 80, contactPositionY);
                    gLabels[LABEL_HISTORY_CONTACT_NAME].render();
                }
            }

        }
        else if (gCurrentPage == PAGE_SETTINGS)
        {
            gLabels[LABEL_TITLE].loadFromText("Settings", fontLato, &colorWhite);

            SDL_Rect renderQuad = {0, 0, 1280 / 6, 960 / 6};

            renderQuad.x = 100; renderQuad.y = 200;
            gButtonsSettingBackgrounds[BGR_1].render(NULL, &renderQuad);

            renderQuad.x = 400; renderQuad.y = 200;
            gButtonsSettingBackgrounds[BGR_2].render(NULL, &renderQuad);

            renderQuad.x = 700; renderQuad.y = 200;
            gButtonsSettingBackgrounds[BGR_3].render(NULL, &renderQuad);

            renderQuad.x = 100; renderQuad.y = 400;
            gButtonsSettingBackgrounds[BGR_4].render(NULL, &renderQuad);

            renderQuad.x = 400; renderQuad.y = 400;
            gButtonsSettingBackgrounds[BGR_5].render(NULL, &renderQuad);
        }
        else if (gCurrentPage == PAGE_FILEBROWSER)
        {
            gLabels[LABEL_TITLE].loadFromText("File Browser", fontLato, &colorWhite);

            FileBrowser::instance()->render();
        }


        gLabels[LABEL_TITLE].setPosition(SCREEN_WIDTH / 2 - gLabels[LABEL_TITLE].getWidth() / 2, 60);
        gLabels[LABEL_TITLE].render();


/*
        gTopIcons[ICO_PLAYER].render();
        gTopIcons[ICO_CONTACTS].render();
        gTopIcons[ICO_MESSAGES].render();
        gTopIcons[ICO_CALLS].render();

        //gTopIcons[ICO_BTN_BT_SYNC].render();

        gImages[IMG_BUTTON_SELECTED].render();
*/
        gTopIcons[ICO_MAIN_MENU].render();


        if (gVisibleOverlays[OVERLAY_MAIN_MANU])
        {
            //gImages[IMG_MAIN_MENU_BACKGROUND].render();

            SDL_Rect fillRect = { 0, 0, gMainMenuBgrWidth, SCREEN_HEIGHT };
            SDL_SetRenderDrawColor( gRenderer, 0x22, 0x22, 0x44, 0xaa );
            SDL_RenderFillRect( gRenderer, &fillRect );

            if (!gAnimations.isAnimating(&gMainMenuBgrWidth))
            {
                int arrowX = gImages[IMG_MAIN_MENU_SEPARATOR].getWidth() - 50 - gImages[IMG_ARROW_RIGHT].getWidth();

                gMainMenuButtons[MAIN_MENU_BUTTON_FILEBROWSER].render();
                gImages[IMG_ARROW_RIGHT].setPosition(arrowX, gMainMenuButtons[MAIN_MENU_BUTTON_FILEBROWSER].getPositionY() + 6);    // TODO: do proper vert centering
                gImages[IMG_ARROW_RIGHT].render();
                gImages[IMG_MAIN_MENU_SEPARATOR].setPosition(0, gMainMenuButtons[MAIN_MENU_BUTTON_FILEBROWSER].getPositionY() + 50);    // TODO: do proper vert centering
                gImages[IMG_MAIN_MENU_SEPARATOR].render();

                gMainMenuButtons[MAIN_MENU_BUTTON_PLAYER].render();
                gImages[IMG_ARROW_RIGHT].setPosition(arrowX, gMainMenuButtons[MAIN_MENU_BUTTON_PLAYER].getPositionY() + 6);    // TODO: do proper vert centering
                gImages[IMG_ARROW_RIGHT].render();
                gImages[IMG_MAIN_MENU_SEPARATOR].setPosition(0, gMainMenuButtons[MAIN_MENU_BUTTON_PLAYER].getPositionY() + 50);    // TODO: do proper vert centering
                gImages[IMG_MAIN_MENU_SEPARATOR].render();

                gMainMenuButtons[MAIN_MENU_BUTTON_MESSAGES].render();
                gImages[IMG_ARROW_RIGHT].setPosition(arrowX, gMainMenuButtons[MAIN_MENU_BUTTON_MESSAGES].getPositionY() + 6);
                gImages[IMG_ARROW_RIGHT].render();
                gImages[IMG_MAIN_MENU_SEPARATOR].setPosition(0, gMainMenuButtons[MAIN_MENU_BUTTON_MESSAGES].getPositionY() + 50);    // TODO: do proper vert centering
                gImages[IMG_MAIN_MENU_SEPARATOR].render();

                gMainMenuButtons[MAIN_MENU_BUTTON_CALLS].render();
                gImages[IMG_ARROW_RIGHT].setPosition(arrowX, gMainMenuButtons[MAIN_MENU_BUTTON_CALLS].getPositionY() + 6);
                gImages[IMG_ARROW_RIGHT].render();
                gImages[IMG_MAIN_MENU_SEPARATOR].setPosition(0, gMainMenuButtons[MAIN_MENU_BUTTON_CALLS].getPositionY() + 50);    // TODO: do proper vert centering
                gImages[IMG_MAIN_MENU_SEPARATOR].render();

                gMainMenuButtons[MAIN_MENU_BUTTON_CONTACTS].render();
                gImages[IMG_ARROW_RIGHT].setPosition(arrowX, gMainMenuButtons[MAIN_MENU_BUTTON_CONTACTS].getPositionY() + 6);
                gImages[IMG_ARROW_RIGHT].render();
                gImages[IMG_MAIN_MENU_SEPARATOR].setPosition(0, gMainMenuButtons[MAIN_MENU_BUTTON_CONTACTS].getPositionY() + 50);    // TODO: do proper vert centering
                gImages[IMG_MAIN_MENU_SEPARATOR].render();

                gMainMenuButtons[MAIN_MENU_BUTTON_SETTINGS].render();
                gImages[IMG_ARROW_RIGHT].setPosition(arrowX, gMainMenuButtons[MAIN_MENU_BUTTON_SETTINGS].getPositionY() + 6);
                gImages[IMG_ARROW_RIGHT].render();
                gImages[IMG_MAIN_MENU_SEPARATOR].setPosition(0, gMainMenuButtons[MAIN_MENU_BUTTON_SETTINGS].getPositionY() + 50);    // TODO: do proper vert centering
                gImages[IMG_MAIN_MENU_SEPARATOR].render();

                gMainMenuButtons[MAIN_MENU_BUTTON_BT_SYNC].render();
                gImages[IMG_ARROW_RIGHT].setPosition(arrowX, gMainMenuButtons[MAIN_MENU_BUTTON_BT_SYNC].getPositionY() + 6);
                gImages[IMG_ARROW_RIGHT].render();
            }

        }


        // FPS
        char fpsBuf[3];
        memset(&fpsBuf, 0, sizeof fpsBuf);
        sprintf(fpsBuf, "%d", gFPS);

        gLabels[LABEL_FPS].loadFromText("FPS: " + string(fpsBuf), fontArial, &colorWhite);
        gLabels[LABEL_FPS].setPosition(SCREEN_WIDTH - gLabels[LABEL_FPS].getWidth() - 20, SCREEN_HEIGHT - gLabels[LABEL_FPS].getHeight() - 20);
        gLabels[LABEL_FPS].render();


        Touch::instance()->main();
        gAnimations.main();


        gFPSCounter++;

        SDL_RenderPresent( gRenderer );
    }

    sqlite3_close(gDB);

    // TODO: kill threads???

    return 0;
}


void executeDBQuery(string query)
{
    char *zErrMsg = 0;
    int rc = 0;

    rc = sqlite3_exec(gDB, query.c_str(), NULL, 0, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }

}

// TODO: dafaaaaaaaaaak! this should be inside filebrowser.cpp
void fileBrowser_clickFile(Button *button)
{
    //printf("fileBrowser_clickFile\n");

    string tracks[] = {FileBrowser::instance()->getCurrentPath() + "/" + button->getTextLabel()};
    gAudioPlayer.loadTracks(tracks, 1);
    gAudioPlayer.play();
}

void btnPlayer_Click(Button *button)
{
    gCurrentPage = PAGE_PLAYER;

    auto callback = []() {
        gVisibleOverlays[OVERLAY_MAIN_MANU] = false;
    };
    gAnimations.addAnimation(&gMainMenuBgrWidth, 0, 70, callback);


/*
    DIR *dpdf;
    struct dirent *epdf;

    string path = "/media/lacho3/Data/Music/Beatport/";
    string pathAndFile;
    string paths[10];

    dpdf = opendir(path.c_str());
    int i = 0;
    if (dpdf != NULL)
    {
        while ((epdf = readdir(dpdf)))
        {
            if (*epdf->d_name == '.' || strcmp(epdf->d_name, "..") == 0)
                continue;

            pathAndFile = path + string(epdf->d_name);
            paths[i++] = pathAndFile;

        }
    }

    gAudioPlayer.loadTracks(paths, i);*/
}

void btnMessages_Click(Button *button)
{
    gCurrentPage = PAGE_MESSAGES;
    gVisibleOverlays[OVERLAY_MAIN_MANU] = false;
}

void btnContacts_Click(Button *button)
{
    gCurrentPage = PAGE_CONTACTS;

    Touch::instance()->reset();
    Touch::instance()->setXBoundaries(10, (gContacts.size() / CONTACTS_CNT_ITEMS_PER_ROW + 2) * (CONTACTS_BUTTON_WIDTH + CONTACTS_BUTTON_HSPACING) - SCREEN_WIDTH);

    gVisibleOverlays[OVERLAY_MAIN_MANU] = false;

    //gDragPrevYPos = 0;
    //gDragPrevXPos = 0;
    //Touch::instance()->resetDragPositions();
}

void btnCalls_Click(Button *button)
{
    gCurrentPage = PAGE_CALLS;

    Touch::instance()->reset();
    gVisibleOverlays[OVERLAY_MAIN_MANU] = false;

/*
    gImages[IMG_BUTTON_SELECTED].setPosition(
        button->getPositionX() - gImages[IMG_BUTTON_SELECTED].getWidth() / 2 + button->getWidth() / 2,
        button->getPositionY() - gImages[IMG_BUTTON_SELECTED].getHeight() / 2 + button->getHeight() / 2);
*/
    // TODO: prev positions should be in touch class!
    //gDragPrevYPos = 0;
    //gDragPrevXPos = 0;
    //Touch::instance()->resetDragPositions();
}
void btnSettings_Click(Button *button)
{
    gCurrentPage = PAGE_SETTINGS;
    gVisibleOverlays[OVERLAY_MAIN_MANU] = false;
}

void btnMainMenu_MouseUp(Button *button)
{
    if (gVisibleOverlays[OVERLAY_MAIN_MANU] == false)
    {
        gVisibleOverlays[OVERLAY_MAIN_MANU] = true;
        gAnimations.addAnimation(&gMainMenuBgrWidth, 0, gImages[IMG_MAIN_MENU_SEPARATOR].getWidth(), 50, NULL);
    }
}

void btnSettings_bgr1_Click(Button *button)
{
	gImages[IMG_BACKGROUND].loadFromFile("resources/images/background1.jpg");

    executeDBQuery("UPDATE settings SET value='background1.jpg' WHERE key='background_image'");
}
void btnSettings_bgr2_Click(Button *button)
{
	gImages[IMG_BACKGROUND].loadFromFile("resources/images/background2.jpg");

    executeDBQuery("UPDATE settings SET value='background2.jpg' WHERE key='background_image'");
}
void btnSettings_bgr3_Click(Button *button)
{
	gImages[IMG_BACKGROUND].loadFromFile("resources/images/background3.jpg");

    executeDBQuery("UPDATE settings SET value='background3.jpg' WHERE key='background_image'");
}
void btnSettings_bgr4_Click(Button *button)
{
	gImages[IMG_BACKGROUND].loadFromFile("resources/images/background4.jpg");

    executeDBQuery("UPDATE settings SET value='background4.jpg' WHERE key='background_image'");
}
void btnSettings_bgr5_Click(Button *button)
{
	gImages[IMG_BACKGROUND].loadFromFile("resources/images/background5.jpg");

    executeDBQuery("UPDATE settings SET value='background5.jpg' WHERE key='background_image'");
}

int threadBluetoothConnect(void *data)
{
    PhoneLib::instance()->connect();

    return 0;
}
int threadGetPlayStatus(void *data)
{
    while (wt32i.isSynchronizing());

    wt32i.requestPlayStatus();

    return 0;
}
int threadGetTrackInfo(void *data)
{
    while (wt32i.isSynchronizing());

    wt32i.requestTrackInfo();

    return 0;
}
int threadWT32WriteRaw(void *data)
{
    while (wt32i.isSynchronizing());

    wt32i.writeRaw((char *)data);

    return 0;
}
int threadWT32GetCallHistory(void *data)
{
    //while (wt32i.isSynchronizing());

    //wt32i.getCallHistory(10, &gCallHistory);
    PhoneLib::instance()->getCallHistory(100, &gCallHistory);

    return 0;
}

int threadWT32GetContacts(void *data)
{
    //while (wt32i.isSynchronizing());

    vector<VCard_t> contacts;
    PhoneLib::instance()->getContacts(1000, &contacts);  // TODO: 1000 limit?


    if (contacts.size() < 1)
    {
        cout << "Error getting contacts (got 0) " << DUMP_FILE_LINE << endl;
        return 0;
    }

    char *zErrMsg = 0;
    int rc = 0;

    rc = sqlite3_exec(gDB, "DELETE FROM contacts", NULL, 0, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }


    string sql = "INSERT INTO contacts (name, phone_cell, phone_home) VALUES ";

    int cntContacts = contacts.size();
    for (int i = 0; i < cntContacts; i++)
    {
        if (contacts[i].name.length() == 0 && (contacts[i].cellPhone.length() == 0 && contacts[i].homePhone.length() == 0))
            break;
        else if (contacts[i].name.length() == 0)
            continue;

        if (i > 0)
            sql += ", ";

        // TODO: escape quotes??
        sql += "('" + contacts[i].name + "', '" + contacts[i].cellPhone + "', '" + contacts[i].homePhone + "')";

    }

    rc = sqlite3_exec(gDB, sql.c_str(), NULL, 0, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else
    {
        cout << "Contacts added in db" << endl;
    }

    loadContactsFromDB();


    return 0;
}

void btnBtSyncClick(Button *button)
{
    if (wt32i.isSynchronizing())
        return;

    SDL_CreateThread(threadWT32GetCallHistory, NULL, NULL);
    SDL_CreateThread(threadWT32GetContacts, NULL, NULL);
}

void btnFileBrowserClick(Button *button)
{
    gCurrentPage = PAGE_FILEBROWSER;

    auto callback = []() {
        gVisibleOverlays[OVERLAY_MAIN_MANU] = false;
    };
    gAnimations.addAnimation(&gMainMenuBgrWidth, 0, 70, callback);

    Touch::instance()->reset();
    FileBrowser::instance()->openFolder("/");
}

void wt32i_onTrackInfoReceive(string artistName, string trackName, string playingTime)
{
    if (artistName.compare("N/A") == 0 && trackName.find("-") != string::npos)
    {
        // TODO: when more than one - exists?
        int dashPos = trackName.find("-");
        gArtistName = trackName.substr(0, dashPos);
        gTrackName = trackName.substr(dashPos + 1, trackName.length() - dashPos);
    }
    else
    {
        gArtistName = artistName;
        gTrackName = trackName;
    }

    int iPlayingTime = atoi(playingTime.c_str());
    if (iPlayingTime > 100000)
        iPlayingTime = iPlayingTime / 1000; // some players give time in milliseconds

    gPlayerTrackTotalPlayingMinutes = iPlayingTime / 60;
    gPlayerTrackTotalPlayingSeconds = iPlayingTime % 60;
}

void wt32i_onPlayStatusReceive(string currentPlayingTime, string isPlaying, string param3)
{
    int iCurrentPlayingTime = (int)strtol(currentPlayingTime.c_str(), NULL, 16) / 1000;

    gPlayerTrackCurrentPlayingMinutes = iCurrentPlayingTime / 60;
	gPlayerTrackCurrentPlayingSeconds = iCurrentPlayingTime % 60;
/*
	if (isPlaying.compare("0") == 0)
        gPLayerIsPlaying = false;
    else
        gPLayerIsPlaying = true;*/
}

void btnPlayerPrevious_Click(Button *button)
{
    printf("btnPlayerPrevious_Click\n");

/*
    if (wt32i.isSynchronizing())
        return;

    SDL_CreateThread(threadWT32WriteRaw, NULL, (void *)"AVRCP BACKWARD\n");

*/
    gPlayerTrackCurrentPlayingMinutes = 0;
    gPlayerTrackCurrentPlayingSeconds = 0;

    gAudioPlayer.playPreviousTrack();

    gArtistName = gAudioPlayer.getCurrentTrackInfo()->artist;
    gTrackName = gAudioPlayer.getCurrentTrackInfo()->title;

    gPlayerTrackTotalPlayingMinutes = gAudioPlayer.getCurrentTrackInfo()->lengthMinutes;
    gPlayerTrackTotalPlayingSeconds = gAudioPlayer.getCurrentTrackInfo()->lengthSeconds;

    //usleep(700000);
    //SDL_CreateThread(threadGetTrackInfo, NULL, NULL);
}

void btnPlayerPlay_Click(Button *button)
{
    printf("btnPlayerPlay_Click\n");

    /*
    if (wt32i.isSynchronizing())
        return;

    SDL_CreateThread(threadWT32WriteRaw, NULL, (void *)"AVRCP PLAY\n");
    usleep(700000);
    SDL_CreateThread(threadGetTrackInfo, NULL, NULL);
    */

    if (gAudioPlayer.getPlayerStatus() == AudioPlayer::PLAYER_STATUS_PLAYING)
        gAudioPlayer.pause();
    else if (gAudioPlayer.getPlayerStatus() == AudioPlayer::PLAYER_STATUS_PAUSED)
        gAudioPlayer.resume();
    else if (gAudioPlayer.getPlayerStatus() == AudioPlayer::PLAYER_STATUS_STOPPED)
    {
        gAudioPlayer.play();
        gArtistName = gAudioPlayer.getCurrentTrackInfo()->artist;
        gTrackName = gAudioPlayer.getCurrentTrackInfo()->title;

        gPlayerTrackTotalPlayingMinutes = gAudioPlayer.getCurrentTrackInfo()->lengthMinutes;
        gPlayerTrackTotalPlayingSeconds = gAudioPlayer.getCurrentTrackInfo()->lengthSeconds;

    }


}

void btnPlayerNext_MouseDown(Button *button)
{
    printf("btnPlayerNext_MouseDown\n");

    /*
    if (wt32i.isSynchronizing())
        return;

    SDL_CreateThread(threadWT32WriteRaw, NULL, (void *)"AVRCP FORWARD\n");

*/
    gPlayerTrackCurrentPlayingMinutes = 0;
    gPlayerTrackCurrentPlayingSeconds = 0;

    gAudioPlayer.playNextTrack();

    gArtistName = gAudioPlayer.getCurrentTrackInfo()->artist;
    gTrackName = gAudioPlayer.getCurrentTrackInfo()->title;

    gPlayerTrackTotalPlayingMinutes = gAudioPlayer.getCurrentTrackInfo()->lengthMinutes;
    gPlayerTrackTotalPlayingSeconds = gAudioPlayer.getCurrentTrackInfo()->lengthSeconds;

    //usleep(700000);
    //SDL_CreateThread(threadGetTrackInfo, NULL, NULL);
}

void btnPlayerNext_MouseUp(Button *button)
{
    printf("btnPlayerNext_MouseUp\n");
}


bool loadMedia()
{
	//Loading success flag
	bool success = true;

    fontLato = TTF_OpenFont("resources/fonts/Lato-Regular.ttf", 28);
    fontArial = TTF_OpenFont("resources/fonts/Arial.ttf", 24);    // TODO: use font from PSD (to support Unicode!)

    FileBrowser::instance()->setRenderer(gRenderer);

    gLabels[LABEL_PLAYER_TRACK_NAME].setRenderer(gRenderer);
    gLabels[LABEL_PLAYER_TRACK_NAME2].setRenderer(gRenderer);
    gLabels[LABEL_PLAYER_ARTIST].setRenderer(gRenderer);
    gLabels[LABEL_FPS].setRenderer(gRenderer);
    gLabels[LABEL_HISTORY_CONTACT_NAME].setRenderer(gRenderer);

	gImages[IMG_BACKGROUND].setRenderer(gRenderer);
	//gImages[IMG_BACKGROUND].loadFromFile("resources/images/background4.jpg");
    gImages[IMG_BACKGROUND].setPosition(0, 0);


    char *zErrMsg = 0;
    int rc = sqlite3_exec(gDB, "SELECT key,value FROM settings", sqliteLoadSettings, 0, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }


    gButtonsSettingBackgrounds[BGR_1].setRenderer(gRenderer);
	gButtonsSettingBackgrounds[BGR_1].loadFromFile("resources/images/background1.jpg");
    gButtonsSettingBackgrounds[BGR_1].setEventHandler(MOUSEBUTTONUP, btnSettings_bgr1_Click);

    gButtonsSettingBackgrounds[BGR_2].setRenderer(gRenderer);
	gButtonsSettingBackgrounds[BGR_2].loadFromFile("resources/images/background2.jpg");
    gButtonsSettingBackgrounds[BGR_2].setEventHandler(MOUSEBUTTONUP, btnSettings_bgr2_Click);

    gButtonsSettingBackgrounds[BGR_3].setRenderer(gRenderer);
	gButtonsSettingBackgrounds[BGR_3].loadFromFile("resources/images/background3.jpg");
    gButtonsSettingBackgrounds[BGR_3].setEventHandler(MOUSEBUTTONUP, btnSettings_bgr3_Click);

    gButtonsSettingBackgrounds[BGR_4].setRenderer(gRenderer);
	gButtonsSettingBackgrounds[BGR_4].loadFromFile("resources/images/background4.jpg");
    gButtonsSettingBackgrounds[BGR_4].setEventHandler(MOUSEBUTTONUP, btnSettings_bgr4_Click);

    gButtonsSettingBackgrounds[BGR_5].setRenderer(gRenderer);
	gButtonsSettingBackgrounds[BGR_5].loadFromFile("resources/images/background5.jpg");
    gButtonsSettingBackgrounds[BGR_5].setEventHandler(MOUSEBUTTONUP, btnSettings_bgr5_Click);

	//gImages[IMG_BACKGROUND2].setRenderer(gRenderer);
	//gImages[IMG_BACKGROUND2].loadFromFile("resources/images/background3.jpg");

	//gImages[IMG_BUTTON_H_SEPARATOR].loadFromFile("resources/images/ico_separator.png");

/*
	gImages[IMG_BUTTON_SELECTED].setRenderer(gRenderer);
	gImages[IMG_BUTTON_SELECTED].loadFromFile("resources/images/ico_btn_selected.png");
    gImages[IMG_BUTTON_SELECTED].setPosition(-100, -100);

	gTopIcons[ICO_PLAYER].setRenderer(gRenderer);
    gTopIcons[ICO_PLAYER].loadFromFile("resources/images/ico_player.png");
    gTopIcons[ICO_PLAYER].setEventHandler(MOUSEBUTTONUP, btnPlayer_Click);

	gTopIcons[ICO_CONTACTS].setRenderer(gRenderer);
    gTopIcons[ICO_CONTACTS].loadFromFile("resources/images/ico_contacts.png");
    gTopIcons[ICO_CONTACTS].setEventHandler(MOUSEBUTTONUP, btnContacts_Click);

	gTopIcons[ICO_MESSAGES].setRenderer(gRenderer);
    gTopIcons[ICO_MESSAGES].loadFromFile("resources/images/ico_messages.png");
    gTopIcons[ICO_MESSAGES].setEventHandler(MOUSEBUTTONUP, btnMessages_Click);

	gTopIcons[ICO_CALLS].setRenderer(gRenderer);
    gTopIcons[ICO_CALLS].loadFromFile("resources/images/ico_phone.png");
    gTopIcons[ICO_CALLS].setEventHandler(MOUSEBUTTONUP, btnCalls_Click);

    gTopIcons[ICO_BTN_BT_SYNC].setRenderer(gRenderer);
    gTopIcons[ICO_BTN_BT_SYNC].loadFromText("BT Sync", fontLato, &colorWhite);
    gTopIcons[ICO_BTN_BT_SYNC].setEventHandler(MOUSEBUTTONUP, btnBtSyncClick);

*/
	gImages[IMG_TOP_LINE].setRenderer(gRenderer);
	gImages[IMG_TOP_LINE].loadFromFile("resources/images/top_line.png");
    gImages[IMG_TOP_LINE].setPosition(
        (SCREEN_WIDTH / 2) - (gImages[IMG_TOP_LINE].getWidth() / 2),
        110);

    gTopIcons[ICO_MAIN_MENU].setRenderer(gRenderer);
    gTopIcons[ICO_MAIN_MENU].loadFromFile("resources/images/ico_menu.png");
    gTopIcons[ICO_MAIN_MENU].setEventHandler(MOUSEBUTTONUP, btnMainMenu_MouseUp);
    gTopIcons[ICO_MAIN_MENU].setPosition(50, 60);



/*
    gTopIcons[ICO_PLAYER].setPosition(
        (SCREEN_WIDTH / 2) - (gTopIcons[ICO_PLAYER].getWidth() / 2) - gTopIcons[ICO_MESSAGES].getWidth() - 60,
        160 - gTopIcons[ICO_PLAYER].getHeight() / 2);

    gTopIcons[ICO_MESSAGES].setPosition(
        (SCREEN_WIDTH / 2) - (gTopIcons[ICO_MESSAGES].getWidth() / 2),
        160 - gTopIcons[ICO_MESSAGES].getHeight() / 2);

    gTopIcons[ICO_CONTACTS].setPosition(
        (SCREEN_WIDTH / 2) - (gTopIcons[ICO_CONTACTS].getWidth() / 2) + gTopIcons[ICO_MESSAGES].getWidth() + 60,
        160 - gTopIcons[ICO_CONTACTS].getHeight() / 2);


    gTopIcons[ICO_CALLS].setPosition(
        (SCREEN_WIDTH / 2) - (gTopIcons[ICO_CALLS].getWidth() / 2) + gTopIcons[ICO_CONTACTS].getWidth() + 120,
        160 - gTopIcons[ICO_CALLS].getHeight() / 2);
*/
/*
    gTopIcons[ICO_PLAYER].setPosition(50, 200);
    gTopIcons[ICO_MESSAGES].setPosition(50, 260);
    gTopIcons[ICO_CONTACTS].setPosition(50, 320);
    gTopIcons[ICO_CALLS].setPosition(50, 380);
*/

    gPlayerButtons[BTN_PLAYER_SHUFFLE].setRenderer(gRenderer);
    gPlayerButtons[BTN_PLAYER_SHUFFLE].loadFromFile("resources/images/ico_player_shuffle.png");
    gPlayerButtons[BTN_PLAYER_SHUFFLE].setPosition(200, SCREEN_HEIGHT - PLAYER_BUTTONS_BOTTOM_SPACING - gPlayerButtons[BTN_PLAYER_SHUFFLE].getHeight() / 2);

    gPlayerButtons[BTN_PLAYER_REPEAT].setRenderer(gRenderer);
    gPlayerButtons[BTN_PLAYER_REPEAT].loadFromFile("resources/images/ico_player_repeat.png");
    gPlayerButtons[BTN_PLAYER_REPEAT].setPosition(270, SCREEN_HEIGHT - PLAYER_BUTTONS_BOTTOM_SPACING - gPlayerButtons[BTN_PLAYER_REPEAT].getHeight() / 2);

    gPlayerButtons[BTN_PLAYER_PLAY].setRenderer(gRenderer);
    gPlayerButtons[BTN_PLAYER_PLAY].loadFromFile("resources/images/ico_player_pause.png");
    gPlayerButtons[BTN_PLAYER_PLAY].setEventHandler(MOUSEBUTTONDOWN, btnPlayerPlay_Click);
    gPlayerButtons[BTN_PLAYER_PLAY].setPosition(
        SCREEN_WIDTH / 2 - gPlayerButtons[BTN_PLAYER_PLAY].getWidth() / 2,
        SCREEN_HEIGHT - PLAYER_BUTTONS_BOTTOM_SPACING - gPlayerButtons[BTN_PLAYER_PLAY].getHeight() / 2);

    gPlayerButtons[BTN_PLAYER_PREVIOUS].setRenderer(gRenderer);
    gPlayerButtons[BTN_PLAYER_PREVIOUS].loadFromFile("resources/images/ico_player_previous.png");
    gPlayerButtons[BTN_PLAYER_PREVIOUS].setEventHandler(MOUSEBUTTONDOWN, btnPlayerPrevious_Click);
    gPlayerButtons[BTN_PLAYER_PREVIOUS].setPosition(
        gPlayerButtons[BTN_PLAYER_PLAY].getPositionX() - gPlayerButtons[BTN_PLAYER_PLAY].getWidth() + gPlayerButtons[BTN_PLAYER_PREVIOUS].getWidth() / 2- PLAYER_BUTTONS_H_SPACING,
        SCREEN_HEIGHT - PLAYER_BUTTONS_BOTTOM_SPACING - gPlayerButtons[BTN_PLAYER_PREVIOUS].getHeight() / 2);

    gPlayerButtons[BTN_PLAYER_NEXT].setRenderer(gRenderer);
    gPlayerButtons[BTN_PLAYER_NEXT].loadFromFile("resources/images/ico_player_next.png");
    gPlayerButtons[BTN_PLAYER_NEXT].setEventHandler(MOUSEBUTTONDOWN, btnPlayerNext_MouseDown);
    gPlayerButtons[BTN_PLAYER_NEXT].setEventHandler(MOUSEBUTTONUP, btnPlayerNext_MouseUp);
    gPlayerButtons[BTN_PLAYER_NEXT].setPosition(
        gPlayerButtons[BTN_PLAYER_PLAY].getPositionX() + gPlayerButtons[BTN_PLAYER_PLAY].getWidth() + PLAYER_BUTTONS_H_SPACING,
        SCREEN_HEIGHT - PLAYER_BUTTONS_BOTTOM_SPACING - gPlayerButtons[BTN_PLAYER_NEXT].getHeight() / 2);

    // TODO: use jpg and optimization
    /*gPlayerButtons[PLAYER_POSITION_BACKGROUND].setRenderer(gRenderer);
    gPlayerButtons[PLAYER_POSITION_BACKGROUND].loadFromFile("resources/images/player_position_background.png");
    gPlayerButtons[PLAYER_POSITION_BACKGROUND].setPosition(0, gPlayerButtons[BTN_PLAYER_PLAY].getPositionY() - 100 - gPlayerButtons[PLAYER_POSITION_BACKGROUND].getHeight() / 2);
*/

    gPlayerButtons[PLAYER_POSITION_DOT].setRenderer(gRenderer);
    gPlayerButtons[PLAYER_POSITION_DOT].loadFromFile("resources/images/player_position_dot.png");


    gLabels[LABEL_PLAYER_PLAYING_TIME].setRenderer(gRenderer);
    gLabels[LABEL_PLAYER_PLAYING_TIME].loadFromText("00:00 / 00:00", fontLato, &colorWhite);


    gImages[BGR_WHITE_GREEN].setRenderer(gRenderer);
    gImages[BGR_WHITE_GREEN].loadFromFile("resources/images/bgr_white_green.png");

    gImages[IMG_ARROW_RIGHT].setRenderer(gRenderer);
    gImages[IMG_ARROW_RIGHT].loadFromFile("resources/images/arrow_right.png");

    /*gImages[IMG_MAIN_MENU_BACKGROUND].setRenderer(gRenderer);
    gImages[IMG_MAIN_MENU_BACKGROUND].loadFromFile("resources/images/mainmenu_background.png");
    gImages[IMG_MAIN_MENU_BACKGROUND].setPosition(0, SCREEN_HEIGHT - gImages[IMG_MAIN_MENU_BACKGROUND].getHeight());
*/
    gImages[IMG_MAIN_MENU_SEPARATOR].setRenderer(gRenderer);
    gImages[IMG_MAIN_MENU_SEPARATOR].loadFromFile("resources/images/mainmenu_separator.png");

    gMainMenuButtons[MAIN_MENU_BUTTON_FILEBROWSER].setRenderer(gRenderer);
    gMainMenuButtons[MAIN_MENU_BUTTON_FILEBROWSER].loadFromText("File browser", fontLato, &colorWhite);
    gMainMenuButtons[MAIN_MENU_BUTTON_FILEBROWSER].setPosition(50, SCREEN_HEIGHT - 520);
    gMainMenuButtons[MAIN_MENU_BUTTON_FILEBROWSER].setEventHandler(MOUSEBUTTONUP, btnFileBrowserClick);

    gMainMenuButtons[MAIN_MENU_BUTTON_PLAYER].setRenderer(gRenderer);
    gMainMenuButtons[MAIN_MENU_BUTTON_PLAYER].loadFromText("Music Player", fontLato, &colorWhite);
    gMainMenuButtons[MAIN_MENU_BUTTON_PLAYER].setPosition(50, SCREEN_HEIGHT - 450);
    gMainMenuButtons[MAIN_MENU_BUTTON_PLAYER].setEventHandler(MOUSEBUTTONUP, btnPlayer_Click);

    gMainMenuButtons[MAIN_MENU_BUTTON_MESSAGES].setRenderer(gRenderer);
    gMainMenuButtons[MAIN_MENU_BUTTON_MESSAGES].loadFromText("Messages", fontLato, &colorWhite);
    gMainMenuButtons[MAIN_MENU_BUTTON_MESSAGES].setPosition(50, SCREEN_HEIGHT - 380);
    gMainMenuButtons[MAIN_MENU_BUTTON_MESSAGES].setEventHandler(MOUSEBUTTONUP, btnMessages_Click);

    gMainMenuButtons[MAIN_MENU_BUTTON_CALLS].setRenderer(gRenderer);
    gMainMenuButtons[MAIN_MENU_BUTTON_CALLS].loadFromText("Calls", fontLato, &colorWhite);
    gMainMenuButtons[MAIN_MENU_BUTTON_CALLS].setPosition(50, SCREEN_HEIGHT - 310);
    gMainMenuButtons[MAIN_MENU_BUTTON_CALLS].setEventHandler(MOUSEBUTTONUP, btnCalls_Click);

    gMainMenuButtons[MAIN_MENU_BUTTON_CONTACTS].setRenderer(gRenderer);
    gMainMenuButtons[MAIN_MENU_BUTTON_CONTACTS].loadFromText("Contacts", fontLato, &colorWhite);
    gMainMenuButtons[MAIN_MENU_BUTTON_CONTACTS].setPosition(50, SCREEN_HEIGHT - 240);
    gMainMenuButtons[MAIN_MENU_BUTTON_CONTACTS].setEventHandler(MOUSEBUTTONUP, btnContacts_Click);

    gMainMenuButtons[MAIN_MENU_BUTTON_SETTINGS].setRenderer(gRenderer);
    gMainMenuButtons[MAIN_MENU_BUTTON_SETTINGS].loadFromText("Settings", fontLato, &colorWhite);
    gMainMenuButtons[MAIN_MENU_BUTTON_SETTINGS].setPosition(50, SCREEN_HEIGHT - 170);
    gMainMenuButtons[MAIN_MENU_BUTTON_SETTINGS].setEventHandler(MOUSEBUTTONUP, btnSettings_Click);

    gMainMenuButtons[MAIN_MENU_BUTTON_BT_SYNC].setRenderer(gRenderer);
    gMainMenuButtons[MAIN_MENU_BUTTON_BT_SYNC].loadFromText("BT Sync", fontLato, &colorWhite);
    gMainMenuButtons[MAIN_MENU_BUTTON_BT_SYNC].setPosition(50, SCREEN_HEIGHT - 100);
    gMainMenuButtons[MAIN_MENU_BUTTON_BT_SYNC].setEventHandler(MOUSEBUTTONUP, btnBtSyncClick);


    gLabels[LABEL_TITLE].setRenderer(gRenderer);
/*
	if( !gBackgroundImage2.loadFromFile( "resources/images/back2.png" ) )
	{
		success = false;
	}

	if( !gButton1.loadFromFile( "resources/images/icons.png" ) )
	{
		success = false;
	}
*/
	return success;
}


static int sqliteLoadSettings(void *notUsed, int argc, char **argv, char **azColName)
{
    string key = argv[0];
    string value = argv[1];

    if (key == "background_image")
        gImages[IMG_BACKGROUND].loadFromFile("resources/images/" + value);

    return 0;
}

static int sqliteParseResponse(void *notUsed, int argc, char **argv, char **azColName)
{
    string name = argv[0];
    string phone_cell = argv[1];
    string phone_home = argv[2];

    VCard_t vcard;
    vcard.name = name;
    vcard.cellPhone = phone_cell;
    vcard.homePhone = phone_home;

    gContacts.push_back(vcard);

    return 0;
}

bool loadContactsFromDB()
{
    gContacts.clear();

    char *zErrMsg = 0;
    int rc;

    string sql = "SELECT t.name, t.phone_cell, t.phone_home FROM contacts t ORDER BY t.name ASC";
    rc = sqlite3_exec(gDB, sql.c_str(), sqliteParseResponse, 0, &zErrMsg);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }

    return true;
}

Uint32 timer_1sCallback(Uint32 interval, void* param)
{
    gFPS = gFPSCounter;
    gFPSCounter = 0;


    if (gAudioPlayer.getPlayerStatus() == AudioPlayer::PLAYER_STATUS_PLAYING)
        gPlayerTrackCurrentPlayingSeconds++;

    if (gPlayerTrackCurrentPlayingSeconds == 60)
    {
        gPlayerTrackCurrentPlayingMinutes++;
        gPlayerTrackCurrentPlayingSeconds = 0;
    }

    // reached next track, get info
    if (gAudioPlayer.getPlayerStatus() == AudioPlayer::PLAYER_STATUS_PLAYING && gPlayerTrackCurrentPlayingMinutes == gPlayerTrackTotalPlayingMinutes && gPlayerTrackCurrentPlayingSeconds > gPlayerTrackTotalPlayingSeconds)
    {
        gPlayerTrackCurrentPlayingMinutes = 0;
        gPlayerTrackCurrentPlayingSeconds = 0;
        wt32i.requestTrackInfo();
    }

    return interval;
}


Uint32 timer_10sCallback(Uint32 interval, void* param)
{
    SDL_CreateThread(threadGetTrackInfo, NULL, NULL);
    SDL_CreateThread(threadGetPlayStatus, NULL, NULL);

    return interval;
}

Uint32 timer_33fpsCallback(Uint32 interval, void* param)
{
    gVideo.loadNextFrame();

    return interval;
}


bool init()
{
	//Initialization flag
	bool success = true;

    //Initialize all SDL subsystems
    if( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER ) == -1 )
    {
        return false;
    }

    if (gIsBoard)
        gWindow = SDL_CreateWindow( "Car media", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP );
    else
        gWindow = SDL_CreateWindow( "Car media", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN );

    if( gWindow == NULL )
    {
        printf( "Window could not be created! SDL Error: %s\n", SDL_GetError() );
        success = false;
    }

    gRenderer = SDL_CreateRenderer( gWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC );
    if( gRenderer == NULL )
    {
        printf( "Renderer could not be created! SDL Error: %s\n", SDL_GetError() );
        success = false;
    }

    SDL_SetRenderDrawBlendMode(gRenderer, SDL_BLENDMODE_BLEND);

    //Initialize SDL_ttf
    if( TTF_Init() == -1 )
    {
        printf("TTF_Init failed\n");
        return false;
    }

	int flags = IMG_INIT_PNG;
    int initted = IMG_Init(flags);
    if ((initted & flags) != flags)
    {
        printf("IMG_Init: %s\n", IMG_GetError());
    }



    // Register all video formats and codecs
    av_register_all();



    wt32i.init("/dev/ttyUSB0", B115200);

    wt32i.registerEventHandler(EVENT_ON_TRACK_INFO_RECEIVE, wt32i_onTrackInfoReceive);
    wt32i.registerEventHandler(EVENT_ON_PLAY_STATUS_RECEIVE, wt32i_onPlayStatusReceive);

    loadContactsFromDB();

	return success;
}

void close()
{

    TTF_CloseFont( fontLato );
    TTF_CloseFont( fontArial );

    Mix_Quit();
    TTF_Quit();
	IMG_Quit();
	SDL_Quit();
}
