#ifndef WT32I_H
#define WT32I_H

#include <string>
#include "common.h"

using namespace std;

enum WT32I_EVENTS
{
    EVENT_ON_TRACK_INFO_RECEIVE,
    EVENT_ON_PLAY_STATUS_RECEIVE,
    TOTAL_WT32I_EVENTS
};

// TODO: make arguments list variable
typedef void (*WT32iEventHandler)(string, string, string);

struct WT32CallHistory_t
{
    string name;
    string phone;
    string date;
    string type;
};

class WT32i
{

public:
    WT32i();
    ~WT32i();

    void init(string, int);
    void registerEventHandler(int event, WT32iEventHandler);

    void requestTrackInfo();
    void requestPlayStatus();
    void writeRaw(string);
    void getCallHistory(int, WT32CallHistory_t *);
    void getContacts(int, VCard_t *);
    bool isSynchronizing();

protected:
    int fd;
    int set_interface_attribs (int speed, int parity);
    void set_blocking (int should_block);
    string serialPortRead(bool readOneLine = false);
    void serialPortWrite(string);
    void serialPortProcessOutput(string output);
    WT32iEventHandler eventHandlers[TOTAL_WT32I_EVENTS];
    bool _isSynchronizing;

    string parseArtistName(string);
    string parseTrackName(string);
    string parsePlayingTime(string);
    string parseCurrentPlayingTime(string);
};

#endif
