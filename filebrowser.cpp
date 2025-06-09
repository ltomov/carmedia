#include "filebrowser.h"

void fileBrowser_clickFile(Button *button);

FileBrowser::FileBrowser()
{
    mRenderer = NULL;
    gDragPrevXPos = 0;

    font = TTF_OpenFont("resources/fonts/Arial.ttf", 25);
    color1 = { 255, 255, 255, SDL_ALPHA_OPAQUE };
    color2 = { 255, 255, 100, SDL_ALPHA_OPAQUE };
}

FileBrowser *FileBrowser::instance()
{
    if (!m_instance)
        m_instance = new FileBrowser;

    return m_instance;
}

FileBrowser::~FileBrowser()
{
    TTF_CloseFont(font);
    font = NULL;
}

void FileBrowser::render()
{
    // render the folder path
    for (vector<Button *>::iterator currentPathButtons_it = currentPathButtons.begin(); currentPathButtons_it != currentPathButtons.end(); ++currentPathButtons_it)
    {
        (*currentPathButtons_it)->render();
    }

    // render the files
    int row = -1,
        col = -1;

    for (vector<Button *>::iterator fileNames_it = fileNames.begin(); fileNames_it != fileNames.end(); ++fileNames_it)
    {
        row++;
        if (row % FILEBROWSER_CNT_ITEMS_PER_ROW == 0)
        {
            col++;
            row = 0;
        }

        int positionY = 200 + row * (FILEBROWSER_BUTTON_HEIGHT + FILEBROWSER_BUTTON_VSPACING);
        int positionX =  50 + col * (FILEBROWSER_BUTTON_WIDTH + FILEBROWSER_BUTTON_HSPACING) + Touch::instance()->getDragScreenOffsetX();

        (*fileNames_it)->setPosition(positionX, positionY);


        if ((*fileNames_it)->getPositionX() >= 0 - FILEBROWSER_BUTTON_WIDTH && (*fileNames_it)->getPositionX() < SCREEN_WIDTH
            /*&& (*fileNames_it)->getPositionY() >= 0 && (*fileNames_it)->getPositionY() < SCREEN_HEIGHT*/)
        {
            (*fileNames_it)->render();
        }

    }
}

void FileBrowser::openFolder(string path)
{
    currentPath = normalizePath(path);

    // the folder path buttons
    currentPathButtons.clear();

    vector<std::string> folderPath;
    boost::split(folderPath, currentPath, boost::is_any_of("/"));
    folderPath.erase(folderPath.begin());
    if (folderPath.at(folderPath.size() - 1) == "")
        folderPath.erase(folderPath.end());

    folderPath.insert(folderPath.begin(), "/");


    uint8_t folderPositionInsidePathTree = 0;
    uint16_t folderPathWidth = 0;
    for (vector<string>::iterator folderPath_it = folderPath.begin(); folderPath_it != folderPath.end(); ++folderPath_it)
    {
        string folder = *folderPath_it;

        currentPathButtons.push_back(new Button());
        Button *button = currentPathButtons.back();

        button->setRenderer(mRenderer);
        button->loadFromText(folder, font, &color2);

        button->setPosition(50 + folderPathWidth, 120);
        folderPathWidth += button->getWidth() + 10;

        auto fnClickHandler = [this, folderPositionInsidePathTree](Button *button) -> void
        {
            vector<std::string> folderPath;
            boost::split(folderPath, currentPath, boost::is_any_of("/"));

            string newPath = "/" + join(folderPath.begin(), folderPath.begin() + folderPositionInsidePathTree + 1, string("/"));
            FileBrowser::instance()->openFolder(newPath);
        };
        button->setEventHandler(MOUSEBUTTONUP, fnClickHandler);

        folderPositionInsidePathTree++;
    }


    // the folder contents
    fileNames.clear();

    struct dirent **namelist;

    int cntFiles = scandir(path.c_str(), &namelist, 0, alphasort);
    if (cntFiles < 0)
        perror("scandir");
    else
    {
        for (int n = 0; n < cntFiles; n++)
        {
            if (strcmp(namelist[n]->d_name, ".") == 0)
            {
                free(namelist[n]);
                continue;
            }


            fileNames.push_back(new Button());
            Button *button = fileNames.back();

            button->setRenderer(mRenderer);

            if (namelist[n]->d_type == 0x4)    // folder
            {
                button->loadFromText(string(namelist[n]->d_name), font, &color2, FILEBROWSER_BUTTON_WIDTH, 23);

                auto fnClickHandler = [](Button *button) -> void
                {
                    Touch::instance()->reset();
                    FileBrowser::instance()->openFolder(FileBrowser::instance()->getCurrentPath() + "/" + button->getTextLabel());
                };
                button->setEventHandler(MOUSEBUTTONUP, fnClickHandler);
            }
            else if (namelist[n]->d_type == 0x8)   // file
            {
                button->loadFromText(string(namelist[n]->d_name), font, &color1, FILEBROWSER_BUTTON_WIDTH, 23);
                button->setEventHandler(MOUSEBUTTONUP, fileBrowser_clickFile);
            }

            free(namelist[n]);
        }
        free(namelist);
    }

/*
    struct dirent *epdf;
    DIR *dpdf = opendir(path.c_str());

    if (dpdf != NULL)
    {
        while ((epdf = readdir(dpdf)))
        {
            if (strcmp(epdf->d_name, ".") == 0)
                continue;

            fileNames.push_back(new Button());
            Button *button = fileNames.back();

            button->setRenderer(mRenderer);

            if (epdf->d_type == 0x4)    // folder
            {
                button->loadFromText(string(epdf->d_name), font, &color2);

                auto fnClickHandler = [](Button *button) -> void
                {
                    Touch::instance()->reset();
                    FileBrowser::instance()->openFolder(FileBrowser::instance()->getCurrentPath() + "/" + button->getTextLabel());
                };
                button->setEventHandler(MOUSEBUTTONUP, fnClickHandler);
            }
            else if (epdf->d_type == 0x8)   // file
            {
                button->loadFromText(string(epdf->d_name), font, &color1);
                button->setEventHandler(MOUSEBUTTONUP, fileBrowser_clickFile);
            }
        }
    }
    */

    Touch::instance()->reset();
    Touch::instance()->setXBoundaries(10, ceil(fileNames.size() / FILEBROWSER_CNT_ITEMS_PER_ROW + 1) * (FILEBROWSER_BUTTON_WIDTH + FILEBROWSER_BUTTON_HSPACING) - SCREEN_WIDTH);
}

void FileBrowser::setRenderer(SDL_Renderer *renderer)
{
    mRenderer = renderer;
}

void FileBrowser::handleEvents(SDL_Event *e)
{
    bool eventHandled = false;

    for (vector<Button *>::iterator fileNames_it = fileNames.begin(); fileNames_it != fileNames.end(); ++fileNames_it)
    {
        if ((*fileNames_it)->handleEvent(e))
        {
            eventHandled = true;
            break;
        }

    }

    if (!eventHandled)
    {
        for (vector<Button *>::iterator folderPath_it = currentPathButtons.begin(); folderPath_it != currentPathButtons.end(); ++folderPath_it)
        {
            if ((*folderPath_it)->handleEvent(e))
                break;
        }
    }

}

string FileBrowser::getCurrentPath()
{
    return currentPath;
}
