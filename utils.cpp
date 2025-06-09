#include "utils.h"

void say(string sentence)
{
    string cmd = "./tools/speech.sh " + sentence;
    system(cmd.c_str());
}

// TODO: optimize?
string normalizePath(string path)
{
    vector<std::string> folderPath;
    boost::split(folderPath, path, boost::is_any_of("/"));

    for (vector<string>::iterator folderPath_it = folderPath.begin(); folderPath_it != folderPath.end();)
    {
        string folder = *folderPath_it;
        if (folder == "..")
        {
            folderPath.erase(folderPath_it);
            folderPath_it--;
            folderPath.erase(folderPath_it);
        }
        else
             folderPath_it++;
    }

    string normalizedPath = "/" + join(folderPath.begin(), folderPath.end(), string("/"));
    return normalizedPath;
}

std::vector<std::string> splitByLength(std::string const & s, size_t count)
{
    std::vector<std::string> tokens;
    for (size_t i = 0; i < s.size(); i += count)
    {
        string token = s.substr(i, count);
        tokens.push_back(token);
    }

    return tokens;
}

