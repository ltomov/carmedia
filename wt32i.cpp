#include <iostream>
#include <termios.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include "wt32i.h"
#include <sstream>

#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string/replace.hpp>

using namespace boost;
using namespace std;

// TODO: make singleton
WT32i::WT32i()
{
    fd = 0;
    _isSynchronizing = false;
}

WT32i::~WT32i()
{
    close(fd);
    fd = 0;
}


void WT32i::writeRaw(string data)
{
    if (_isSynchronizing)
        return;
    _isSynchronizing = true;

    serialPortWrite(data);

    _isSynchronizing = false;
}

void WT32i::serialPortWrite(string data)
{
    cout << "[wt32i::serialPortWrite] " << data << endl;

    write(fd, data.c_str(), data.length());
    usleep ((data.length() + 25) * 200);

    //string output = serialPortRead();
    //serialPortProcessOutput(output);

}

// TODO: rewrite with strings
string WT32i::parseArtistName(string data)
{
    // TODO: make sure this is case sensitive
    size_t artistWordPos = data.find("\" ARTIST ");
    if (artistWordPos == string::npos)
        return NULL;

    size_t artistNameFirstQuotePos = data.find('"', artistWordPos + 5) + 1;
    size_t artistNameSecondQuotePos = data.find('"', artistNameFirstQuotePos);

    if (artistNameSecondQuotePos - artistNameFirstQuotePos < 1)
        return "N/A";

    string artistName = data.substr(artistNameFirstQuotePos, artistNameSecondQuotePos - artistNameFirstQuotePos);

    return artistName;
}

string WT32i::parseTrackName(string data)
{
    // TODO: make sure this is case sensitive

    size_t titleWordPos = data.find(" TITLE ");
    if (titleWordPos == string::npos)
        return NULL;

    size_t titleFirstQuotePos = data.find('"', titleWordPos) + 1;
    size_t titleSecondQuotePos = data.find('"', titleFirstQuotePos);

    if (titleSecondQuotePos - titleFirstQuotePos < 1)
        return "N/A";

    string trackName = data.substr(titleFirstQuotePos, titleSecondQuotePos - titleFirstQuotePos);

    return trackName;
}

string WT32i::parsePlayingTime(string data)
{
    // TODO: make sure this is case sensitive

    size_t playingTimeWordPos = data.find(" PLAYING_TIME ");
    if (playingTimeWordPos == string::npos)
        return NULL;

    size_t playingTimeFirstQuotePos = data.find('"', playingTimeWordPos) + 1;
    size_t playingTimeSecondQuotePos = data.find('"', playingTimeFirstQuotePos);

    string playingTime = data.substr(playingTimeFirstQuotePos, playingTimeSecondQuotePos - playingTimeFirstQuotePos);

    return playingTime;
}

string WT32i::parseCurrentPlayingTime(string data)
{
    size_t markerPos = data.find(" GET_PLAY_STATUS_RSP ");
    if (markerPos == string::npos)
        return NULL;

    size_t timeStartPost = data.find(" ", markerPos + 25) + 1;

    string sCurrentPlayingTime = data.substr(timeStartPost, 8);

    return sCurrentPlayingTime;
}


void WT32i::requestTrackInfo()
{
    if (_isSynchronizing)
        return;
    _isSynchronizing = true;

    serialPortWrite("AVRCP PDU 20 \n");
    serialPortProcessOutput(serialPortRead());

    _isSynchronizing = false;
}

void WT32i::requestPlayStatus()
{
    if (_isSynchronizing)
        return;

    _isSynchronizing = true;

    serialPortWrite("AVRCP PDU 30\n");

    serialPortProcessOutput(serialPortRead());

    _isSynchronizing = false;
}

void WT32i::registerEventHandler(int event, WT32iEventHandler handler)
{
    eventHandlers[event] = handler;
}

void WT32i::serialPortProcessOutput(string output)
{
    // TODO: check which events have handlers and output.find() only for them

    // TODO: count 1 ??
    if (output.find("GET_ELEMENT_ATTRIBUTES_RSP COUNT 7") != string::npos)
    {
        string trackName = parseTrackName(output);
        string artistName = parseArtistName(output);
        string playingTime = parsePlayingTime(output);

        (*eventHandlers[EVENT_ON_TRACK_INFO_RECEIVE])(artistName, trackName, playingTime);
    }
    else if (output.find(" GET_PLAY_STATUS_RSP ") != string::npos)
    {
        string currentPlayingTime = parseCurrentPlayingTime(output);
        string isPlaying = (output.find(" PLAYING") == string::npos) ? "0" : "1";

        (*eventHandlers[EVENT_ON_PLAY_STATUS_RECEIVE])(currentPlayingTime, isPlaying, "");
    }
}

string WT32i::serialPortRead(bool readOneLine)
{
    char buf [1];
    string buf2 = "";
    memset (&buf, 0, sizeof buf);

    bool readCondition;

    int n = 0;
    do
    {
        n = read (fd, buf, 1);
        //printf("%c", buf[0]);
        buf2.append(&buf[0]);

        if (readOneLine)
            readCondition = (buf[0] != '\r' && buf[0] != '\n' && n > 0);
        else
            readCondition = (n > 0);

    } while (readCondition);

    //printf("\r\n");


    return buf2;
}

void WT32i::init(string portName, int baud)
{
    fd = open (portName.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0)
    {
        printf("error %d opening %s: %s\n", errno, portName.c_str(), strerror (errno));
        return;
    }

    set_interface_attribs (baud, 0);  // set speed to 115,200 bps, 8n1 (no parity)
    set_blocking (0);                // set no blocking
}


int WT32i::set_interface_attribs (int speed, int parity)
{
    struct termios tty;
    memset (&tty, 0, sizeof tty);
    if (tcgetattr (fd, &tty) != 0)
    {
        printf ("error %d from tcgetattr\n", errno);
        return -1;
    }

    cfsetospeed (&tty, speed);
    cfsetispeed (&tty, speed);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
    // disable IGNBRK for mismatched speed tests; otherwise receive break
    // as \000 chars
    tty.c_iflag &= ~IGNBRK;         // disable break processing
    tty.c_lflag = 0;                // no signaling chars, no echo,
    // no canonical processing
    tty.c_oflag = 0;                // no remapping, no delays
    tty.c_cc[VMIN]  = 0;            // read doesn't block
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

    tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
    // enable reading
    tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
    tty.c_cflag |= parity;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    if (tcsetattr (fd, TCSANOW, &tty) != 0)
    {
        printf ("error %d from tcsetattr\n", errno);
        return -1;
    }
    return 0;
}

void WT32i::set_blocking (int should_block)
{
    struct termios tty;
    memset (&tty, 0, sizeof tty);
    if (tcgetattr (fd, &tty) != 0)
    {
        printf ("error %d from tggetattr\n", errno);
        return;
    }

    tty.c_cc[VMIN]  = should_block ? 1 : 0;
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

    if (tcsetattr (fd, TCSANOW, &tty) != 0)
        printf ("error %d setting term attributes\n", errno);
}

void WT32i::getCallHistory(int cntEntries, WT32CallHistory_t *history)
{
    if (_isSynchronizing)
        return;

    _isSynchronizing = true;


    char strCntEntries[2];
    sprintf(strCntEntries, "%d", cntEntries);

    serialPortWrite("pbap 04 " + string(strCntEntries) + "\n");

    //while (serialPortRead().length() == 0);
    string output = serialPortRead();

    int i = 0;
    char_separator<char> sep("\n");
    tokenizer<char_separator<char> > tokens(output, sep);
    BOOST_FOREACH(string t, tokens)
    {
        if (t.find("FN;CHARSET=") != string::npos)
        {
            string name = t.substr(17, t.length() - 17);
            history[i].name = name;
        }
        else if (t.find("FN:") != string::npos)
        {
            if (t.length() > 3)
            {
                string name = t.substr(3, t.length() - 3);
                history[i].name = name;
            }
            else
                history[i].name = "N/A";
        }
        else if (t.find("TEL;") != string::npos)
        {
            size_t colonPos = t.find(':');
            string phone = t.substr(colonPos + 1, t.length() - colonPos + 1);

            history[i].phone = phone;
        }
        else if (t.find("CALL-DATETIME") != string::npos)
        {
            size_t colonPos = t.find(':');
            string date = t.substr(colonPos + 1, t.length() - colonPos + 1);

            history[i].date = date;

            if (t.find(";MISSED") != string::npos)
                history[i].type = "MISSED";
            else if (t.find(";DIALED") != string::npos)
                history[i].type = "DIALED";
            else
                history[i].type = "RECEIVED";

            i++;
            // TODO: more entries than specified? dafuq
            if (i == cntEntries)
                break;
        }
    }

    _isSynchronizing = false;

}

void WT32i::getContacts(int cntEntries, VCard_t *contacts)
{
    if (_isSynchronizing)
        return;

    _isSynchronizing = true;

    char strCntEntries[2];
    sprintf(strCntEntries, "%d", cntEntries);

    serialPortWrite("pbap 00 " + string(strCntEntries) + "\n");

    //while (serialPortRead().length() == 0);

    string output = serialPortRead();
    cout << "Contacts received, parsing... " << endl;
    int i = 0;
    char_separator<char> sep("\n");
    tokenizer<char_separator<char> > tokens(output, sep);
    BOOST_FOREACH(string t, tokens)
    {
        if (t.find("N;CHARSET=") != string::npos)
        {
            size_t colonPos = t.find(':');
            string nameEncoded = t.substr(colonPos + 1, t.length() - colonPos + 1);

            unsigned char utfByteArray[50];
            char_separator<char> sep2("=");
            tokenizer<char_separator<char> > tokens2(nameEncoded, sep2);

            memset(&utfByteArray, 0, sizeof utfByteArray);
            int iii = 0;
            BOOST_FOREACH(string t2, tokens2)
            {
                int x;
                stringstream ss;
                ss << std::hex << t2;
                ss >> x;

                utfByteArray[iii++] = x;
            }

            string nameDecoded = string((char *)utfByteArray);

            contacts[i].name = nameDecoded;
        }
        else if (t.find("FN:") != string::npos)
        {
            if (t.length() > 3)
            {
                string name = t.substr(3, t.length() - 3);
                contacts[i].name = name;
            }
            else
                contacts[i].name = "N/A";
        }
        else if (t.find("TEL;CELL") != string::npos)
        {
            size_t colonPos = t.find(':');
            string phone = t.substr(colonPos + 1, t.length() - colonPos + 1);

            contacts[i].cellPhone = phone;
        }
        else if (t.find("TEL;HOME") != string::npos)
        {
            size_t colonPos = t.find(':');
            string phone = t.substr(colonPos + 1, t.length() - colonPos + 1);

            contacts[i].homePhone = phone;
        }
        else if (t.find("END:VCARD") != string::npos)
        {
            if (contacts[i].homePhone.length() > 0 || contacts[i].cellPhone.length() > 0)
            {
                i++;

                // TODO: more entries than specified? dafuq
                if (i == cntEntries)
                    break;
            }

        }
    }

    _isSynchronizing = false;
}

bool WT32i::isSynchronizing()
{
    return _isSynchronizing;
}
