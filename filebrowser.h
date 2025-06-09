#ifndef FILEBROWSER_H
#define FILEBROWSER_H

#include <iostream>
#include <vector>
#include <SDL_ttf.h>
#include <dirent.h>
#include "button.h"
#include "settings.h"
#include "common.h"
#include "touch.h"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

using namespace std;


class FileBrowser
{
public:
    static FileBrowser *instance();
    ~FileBrowser();

    void render();
    void openFolder(string);
    void setRenderer(SDL_Renderer *);
    void handleEvents(SDL_Event *);
    string getCurrentPath();

protected:
    FileBrowser();
    static FileBrowser *m_instance;

    SDL_Renderer *mRenderer;
    vector<Button *> fileNames;
    vector<Button *> currentPathButtons;
    TTF_Font *font;
    SDL_Color color1, color2;
    string currentPath;

    int gDragPrevXPos;
};


#endif

