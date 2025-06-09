#ifndef COMMON_H
#define COMMON_H
// TODO: move in utils.h?

#include <string>
using namespace std;

#define DUMP_EXCEPTION(e)   cout << "Exception: " << e.what() << " (" << __FILE__ << ":" << __LINE__ << ")" << endl;
#define ARRAY_LENGTH(array) ((int)sizeof((array))/(int)sizeof((array)[0]))
#define DUMP_FILE_LINE      __FILE__ << ":" << __LINE__

struct Point
{
    int x;
    int y;
};

// TODO: move to phonelib.h when wt32 is removed
struct VCard_t
{
    string name;
    string cellPhone;
    string homePhone;

    void reset()
    {
        name = "";
        cellPhone = "";
        homePhone = "";
    };
};

#endif
