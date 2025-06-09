#ifndef PHONELIB_H
#define PHONELIB_H

#include <string>
#include <SDL_thread.h>
#include "../common.h"
#include "../utils.h"

#include "proxies/propertiesproxy.h"
#include "proxies/obexclientproxy.h"
#include "proxies/deviceproxy.h"
#include "proxies/objectmanagerproxy.h"
#include "proxies/transferproxy.h"
#include "proxies/mediaplayerproxy.h"
#include "proxies/phonebookaccessproxy.h"


class DBusDevice
: public org::bluez::Device1_proxy,
  public DBus::ObjectProxy
{
public:
    DBusDevice(DBus::Connection& connection, const char* path, const char* name)
    : DBus::ObjectProxy(connection, path, name) {
    }
};


class DBusObjectManager
: public org::freedesktop::DBus::ObjectManager_proxy,
  public DBus::ObjectProxy
{
public:
    DBusObjectManager(DBus::Connection& connection, const char* path, const char* name)
    : DBus::ObjectProxy(connection, path, name) {
    }

    void InterfacesAdded(const ::DBus::Path &object, const std::map< std::string, std::map< std::string, ::DBus::Variant > > &interfaces)
    {
        cout << "DBusObjectManager::InterfacesAdded - " /*<< interfaces.first*/ << endl;
    }

    void InterfacesRemoved(const ::DBus::Path &object, const std::vector< std::string > &interfaces)
    {
        cout << "DBusObjectManager::InterfacesRemoved - " /*<< interfaces.first*/ << endl;
    }
};

class DBusTransfer
: public org::bluez::obex::Transfer1_proxy,
  public DBus::ObjectProxy
{
public:
    DBusTransfer(DBus::Connection& connection, const char* path, const char* name)
    : DBus::ObjectProxy(connection, path, name) {
    }

};



class DBusObexClient
: public org::bluez::obex::Client1_proxy,
  public DBus::ObjectProxy
{
public:
    DBusObexClient(DBus::Connection& connection, const char* path, const char* name)
    : DBus::ObjectProxy(connection, path, name) {
    }
};

class DBusPhonebookAccess
: public org::bluez::obex::PhonebookAccess1_proxy,
  public DBus::ObjectProxy
{
public:
    DBusPhonebookAccess(DBus::Connection& connection, const char* path, const char* name)
    : DBus::ObjectProxy(connection, path, name) {
    }
};

class DBusMediaPlayer
: public org::bluez::MediaPlayer1_proxy,
  public DBus::ObjectProxy
{
public:
    DBusMediaPlayer(DBus::Connection& connection, const char* path, const char* name)
    : DBus::ObjectProxy(connection, path, name) {
    }
};

class DBusProperties
: public org::freedesktop::DBus::Properties_proxy,
  public DBus::ObjectProxy
{
public:
    DBusProperties(DBus::Connection& connection, const char* path, const char* name)
    : DBus::ObjectProxy(connection, path, name) {
    }

    void PropertiesChanged(const std::string &interface, const std::map< std::string, ::DBus::Variant > &changed_properties, const std::vector< std::string > &invalidated_properties)
    {
        cout << "PropertiesChanged: " /*<< *changed_properties.first*/ << endl;
    }

};




struct CallHistory_t
{
    string name;
    string phone;
    string date;
    string type;

    void reset()
    {
        name = "";
        phone = "";
        date = "";
        type = "";
    }
};

struct BluetoothDevice_t
{
    string name;
    string address;
    bool paired;
    bool trusted;
    string dbusPath;
};

class PhoneLib
{
public:
    ~PhoneLib();
    static PhoneLib *instance();

    void init(DBus::Connection *, DBus::Connection *);

    void connect();
    vector<BluetoothDevice_t> getPairedDevices();
    void getCallHistory(int, vector<CallHistory_t> *);
    void getContacts(int, vector<VCard_t> *);
    DBusMediaPlayer *getMediaPlayer();

private:
    PhoneLib();
    static PhoneLib *m_instance;

    bool connected;
    string pbapSession;
    void waitForDbusTransfer(string);   // TODO: move somewhere more global?

    DBus::Connection *dbusSystemBus;
    DBus::Connection *dbusSessionBus;

    DBusMediaPlayer *mediaPlayer;
    DBusProperties *mediaPlayerProperties;

    SDL_SpinLock dbusLock;

    bool gettingContacts;
    bool gettingCalls;


};


#endif
