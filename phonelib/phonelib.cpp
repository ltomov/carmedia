#include "phonelib.h"
#include <fstream>
#include <map>

#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>

using namespace boost;
using namespace std;



PhoneLib::PhoneLib()
{
    connected = false;
    pbapSession = "";

    dbusLock = 0;
    gettingContacts = false;
    gettingCalls = false;

    mediaPlayer = NULL;
    mediaPlayerProperties = NULL;
}

PhoneLib *PhoneLib::instance()
{
    if (!m_instance)
        m_instance = new PhoneLib;

    return m_instance;
}

PhoneLib::~PhoneLib()
{
    mediaPlayer = NULL;
    mediaPlayerProperties = NULL;
}

void PhoneLib::init(DBus::Connection *_dbusSystemBus, DBus::Connection *_dbusSessionBus)
{
    dbusSystemBus = _dbusSystemBus;
    dbusSessionBus = _dbusSessionBus;
}

void PhoneLib::waitForDbusTransfer(string path)
{
    map<::DBus::Path, map<string, map<string, ::DBus::Variant>>> managedObjects;
    DBusObjectManager dbusObjectManager(*dbusSessionBus, "/", "org.bluez.obex");

    do
    {
        managedObjects = dbusObjectManager.GetManagedObjects();
    }
    while (managedObjects.find(path) != managedObjects.end());
}


vector<BluetoothDevice_t> PhoneLib::getPairedDevices()
{
    static vector<BluetoothDevice_t> pairedDevices; // TODO: memleaks?

    map<::DBus::Path, map<string, map<string, ::DBus::Variant>>> managedObjects;

    try
    {
        DBusObjectManager dbusObjectManager(*dbusSystemBus, "/", "org.bluez");
        managedObjects = dbusObjectManager.GetManagedObjects();
    }
    catch (DBus::Error e)
    {
        DUMP_EXCEPTION(e)
    }


    for (map<::DBus::Path, map<string, map<string, ::DBus::Variant>>>::iterator it = managedObjects.begin(); it != managedObjects.end(); ++it)
    {
        string dbusPath = it->first;

        map<string, map<string, ::DBus::Variant>> dbusInterfaces = it->second;
        if (dbusInterfaces.find("org.bluez.Device1") != dbusInterfaces.end())
        {
            map<string, ::DBus::Variant> mapDevice = dbusInterfaces["org.bluez.Device1"];

            BluetoothDevice_t btDevice;

            string name         = mapDevice["Name"];
            string address      = mapDevice["Address"];
            btDevice.name       = name;
            btDevice.address    = address;
            btDevice.paired     = mapDevice["Paired"];
            btDevice.trusted    = mapDevice["Trusted"];
            btDevice.dbusPath   = dbusPath;

            // TODO: and trusted?
            if (btDevice.paired && btDevice.trusted)
            {
                pairedDevices.push_back(btDevice);
                cout << "Found paired device: " << btDevice.name << " (" << btDevice.address << ")" << endl;
            }
        }
    }

    return pairedDevices;
}

void PhoneLib::connect()
{
    try
    {
        vector<BluetoothDevice_t> pairedDevices = getPairedDevices();
        if (pairedDevices.size() == 0)
        {
            cout << "No paired devices found, cannot connect " << DUMP_FILE_LINE << endl;
            return;
        }

        // TODO: don't connect to the first in list, but to last connected
        BluetoothDevice_t pairedDevice = pairedDevices[0];

        cout << "Connecting to " << pairedDevice.name << " (" << pairedDevice.address << ")...";
        cout << " ";

        DBusDevice dbusDevice(*dbusSystemBus, pairedDevice.dbusPath.c_str(), "org.bluez");
        dbusDevice.Connect();
        cout << "done." << endl;

        //say("bluetooth connected to " + dbusDevice.Name());

        DBusObexClient dbusObexClient(*dbusSessionBus, "/org/bluez/obex", "org.bluez.obex");
/*
        ::DBus::Variant v_destination;
        ::DBus::MessageIter m = v_destination.writer();
        m.append_string("CC:FA:00:10:81:8E");
*/
        ::DBus::Variant v_target;
        ::DBus::MessageIter m2 = v_target.writer();
        m2.append_string("PBAP");

        map <string, ::DBus::Variant> sessionParam;
        //sessionParam["Destination"] = v_destination;
        sessionParam["Target"] = v_target;

        ::DBus::Path path = dbusObexClient.CreateSession(dbusDevice.Address(), sessionParam);
        pbapSession = path;

        cout << "session: " << path << endl;



        // init media player

        map<::DBus::Path, map<string, map<string, ::DBus::Variant>>> managedObjects;

        DBusObjectManager dbusObjectManager(*dbusSystemBus, "/", "org.bluez");
        managedObjects = dbusObjectManager.GetManagedObjects();

        for (map<::DBus::Path, map<string, map<string, ::DBus::Variant>>>::iterator it = managedObjects.begin(); it != managedObjects.end(); ++it)
        {
            string dbusPath = it->first;

            map<string, map<string, ::DBus::Variant>> dbusInterfaces = it->second;
            if (dbusInterfaces.find("org.bluez.MediaPlayer1") != dbusInterfaces.end())
            {
                cout << "Found MediaPlayer " << dbusPath << endl;

                static DBusMediaPlayer m(*dbusSystemBus, dbusPath.c_str(), "org.bluez");
                mediaPlayer = &m;

                static DBusProperties p(*dbusSystemBus, dbusPath.c_str(), "org.bluez");
                mediaPlayerProperties = &p;
            }
        }


        connected = true;
    }
    catch (DBus::Error e)
    {
        DUMP_EXCEPTION(e)

        return;
    }
}

// TODO: remove cntEntries?
void PhoneLib::getCallHistory(int cntEntries, vector<CallHistory_t> *history)
{
    if (!connected)
    {
        cout << "Bluetooth not connected " << DUMP_FILE_LINE << endl;
        return;
    }

    if (gettingCalls) return;
    gettingCalls = true;

    const string pbFileName = "/tmp/dbusGetCallHistory";

    try
    {
        SDL_AtomicLock(&dbusLock);

        DBusPhonebookAccess dbusPhoneBookAccess(*dbusSessionBus, pbapSession.c_str(), "org.bluez.obex");

        map<string, DBus::Variant> filters, properties;
        DBus::Path transfer;

        dbusPhoneBookAccess.Select("int", "cch");
        dbusPhoneBookAccess.PullAll(pbFileName, filters, transfer, properties);

        cout << "Transferring " << transfer << "... ";
        waitForDbusTransfer(transfer);
        cout << "done." << endl;

        SDL_AtomicUnlock(&dbusLock);
    }
    catch (DBus::Error e)
    {
        DUMP_EXCEPTION(e)

        SDL_AtomicUnlock(&dbusLock);
        gettingCalls = false;

        return;
    }



    history->clear();
    CallHistory_t historyEntry;

    ifstream file(pbFileName);

    // TODO: parse line by line, not using tokenizer
    string line;
    int i = 0;

    if (file.is_open())
    {
        while (getline(file, line))
        {
            line.erase(remove(line.end() - 3, line.end(), '\r'), line.end());

            if (line.find("FN;CHARSET=") != string::npos)
            {

                string name = line.substr(17, line.length() - 17);
                historyEntry.name = name;
            }
            else if (line.find("FN:") != string::npos)
            {
                if (line.length() > 3)
                {
                    string name = line.substr(3, line.length() - 3);
                    historyEntry.name = name;
                }
            }
            else if (line.find("TEL;") != string::npos)
            {
                size_t colonPos = line.find(':');
                string phone = line.substr(colonPos + 1, line.length() - colonPos + 1);

                historyEntry.phone = phone;
            }
            else if (line.find("CALL-DATETIME") != string::npos)
            {
                size_t colonPos = line.find(':');
                string date = line.substr(colonPos + 1, line.length() - colonPos + 1);

                historyEntry.date = date;

                if (line.find(";MISSED") != string::npos)
                    historyEntry.type = "MISSED";
                else if (line.find(";DIALED") != string::npos)
                    historyEntry.type = "DIALED";
                else
                    historyEntry.type = "RECEIVED";



                // store object

                if (historyEntry.name.length() == 0)
                    historyEntry.name = historyEntry.phone;


                history->push_back(historyEntry);
                historyEntry.reset();

                i++;
                // TODO: more entries than specified? dafuq
                if (i == cntEntries)
                    break;
            }
        }


        file.close();
    }
    else
        cout << "Error: Unable to open phonebook file" << endl;

    if (remove(pbFileName.c_str()) != 0)
        cout << "Error deleting file " + pbFileName << endl;

    gettingCalls = false;
}





// TODO: remove cntEntries?
void PhoneLib::getContacts(int cntEntries, vector<VCard_t> *contacts)
{
    if (!connected)
    {
        cout << "Bluetooth not connected " << DUMP_FILE_LINE << endl;
        return;
    }

    if (gettingContacts) return;
    gettingContacts = true;

    // TODO: need to close any dbus resources?

    const string pbFileName = "/tmp/dbusGetContacts";

    try
    {
        SDL_AtomicLock(&dbusLock);

        DBusPhonebookAccess dbusPhoneBookAccess(*dbusSessionBus, pbapSession.c_str(), "org.bluez.obex");

        map<string, DBus::Variant> filters, properties;
        DBus::Path transfer;

        dbusPhoneBookAccess.Select("int", "pb");
        dbusPhoneBookAccess.PullAll(pbFileName, filters, transfer, properties);

        cout << "Transferring " << transfer << "... ";
        waitForDbusTransfer(transfer);
        cout << "done." << endl;

        SDL_AtomicUnlock(&dbusLock);

    }
    catch (DBus::Error e)
    {
        DUMP_EXCEPTION(e)

        SDL_AtomicUnlock(&dbusLock);
        gettingContacts = false;

        return;
    }



    contacts->clear();
    VCard_t vcardEntry;

    ifstream file(pbFileName);


    // TODO: parse line by line, not using tokenizer
    string line;
    int i = 0;

    if (file.is_open())
    {
        while (getline(file, line))
        {
            // remove \r
            line.erase(remove(line.end() - 3, line.end(), '\r'), line.end());

            if (line.find("FN;CHARSET=") != string::npos)
            {
                size_t colonPos = line.find(':');
                string nameEncoded = line.substr(colonPos + 1, line.length() - colonPos + 1);

                // TODO: this is fkin url_decode!!
                unsigned char utfByteArray[50]; // TODO: 50?
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

                vcardEntry.name = nameDecoded;
            }
            else if (line.find("FN:") != string::npos)
            {
                if (line.length() > 3)
                {
                    string name = line.substr(3, line.length() - 3);
                    vcardEntry.name = name;
                }
                else
                    vcardEntry.name = "N/A";
            }
            else if (line.find("TEL;CELL") != string::npos)
            {
                size_t colonPos = line.find(':');
                string phone = line.substr(colonPos + 1, line.length() - colonPos + 1);

                vcardEntry.cellPhone = phone;
            }
            else if (line.find("TEL;HOME") != string::npos)
            {
                size_t colonPos = line.find(':');
                string phone = line.substr(colonPos + 1, line.length() - colonPos + 1);

                vcardEntry.homePhone = phone;
            }
            else if (line.find("END:VCARD") != string::npos)
            {
                if (vcardEntry.homePhone.length() > 0 || vcardEntry.cellPhone.length() > 0)
                {
                    i++;

                    contacts->push_back(vcardEntry);
                    vcardEntry.reset();

                    // TODO: more entries than specified? dafuq
                    if (i == cntEntries)
                        break;
                }

            }
        }

        file.close();
    }
    else
        cout << "Error: Unable to open phonebook file" << endl;


    if (remove(pbFileName.c_str()) != 0)
        cout << "Error deleting file " + pbFileName << endl;

    gettingContacts = false;
}

DBusMediaPlayer *PhoneLib::getMediaPlayer()
{
    return mediaPlayer;
}
