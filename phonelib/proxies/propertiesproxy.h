#ifndef __dbusxx__propertiesproxy_h__PROXY_MARSHALL_H
#define __dbusxx__propertiesproxy_h__PROXY_MARSHALL_H

#include <dbus-c++/dbus.h>
#include <cassert>

namespace org {
namespace freedesktop {
namespace DBus {

class Properties_proxy
  : public ::DBus::InterfaceProxy
{
public:
    Properties_proxy()
    : ::DBus::InterfaceProxy("org.freedesktop.DBus.Properties")
    {
        connect_signal(Properties_proxy, PropertiesChanged, _PropertiesChanged_stub);
    }

    /* properties exported by this interface */
    /* methods exported by this interface.
     * these functions will invoke the corresponding methods
     * on the remote objects.
     */
    ::DBus::Variant Get(const std::string& interface, const std::string& name)
    {
        ::DBus::CallMessage __call;
        ::DBus::MessageIter __wi = __call.writer();
        __wi << interface;
        __wi << name;
        __call.member("Get");
        ::DBus::Message __ret = invoke_method(__call);
        ::DBus::MessageIter __ri = __ret.reader();
        ::DBus::Variant __argout;
        __ri >> __argout;
        return __argout;
    }

    void Set(const std::string& interface, const std::string& name, const ::DBus::Variant& value)
    {
        ::DBus::CallMessage __call;
        ::DBus::MessageIter __wi = __call.writer();
        __wi << interface;
        __wi << name;
        __wi << value;
        __call.member("Set");
        invoke_method(__call);
    }

    std::map< std::string, ::DBus::Variant > GetAll(const std::string& interface)
    {
        ::DBus::CallMessage __call;
        ::DBus::MessageIter __wi = __call.writer();
        __wi << interface;
        __call.member("GetAll");
        ::DBus::Message __ret = invoke_method(__call);
        ::DBus::MessageIter __ri = __ret.reader();
        std::map< std::string, ::DBus::Variant > __argout;
        __ri >> __argout;
        return __argout;
    }

    /* signal handlers for this interface.
     * you will have to implement them in your ObjectProxy.
     */
    virtual void PropertiesChanged(const std::string &interface, const std::map< std::string, ::DBus::Variant > &changed_properties, const std::vector< std::string > &invalidated_properties) = 0;

protected:
private:
    /* unmarshallers (to unpack the DBus message before
     * calling the actual signal handler)
     */
    void _PropertiesChanged_stub(const ::DBus::SignalMessage &__sig)
    {
        ::DBus::MessageIter __ri = __sig.reader();
        std::string interface; __ri >> interface;
        std::map< std::string, ::DBus::Variant > changed_properties; __ri >> changed_properties;
        std::vector< std::string > invalidated_properties; __ri >> invalidated_properties;
        PropertiesChanged(interface, changed_properties, invalidated_properties);
    }

};
}}}

#endif
