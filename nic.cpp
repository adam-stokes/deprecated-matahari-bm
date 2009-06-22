#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <stdexcept>

#include <hal/libhal.h>
#include <cstdio>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/types.h>
#include <linux/ethtool.h>
#include <linux/sockios.h>
#include <linux/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include "nic.h"
#include "qmf/com/redhat/nodereporter/NIC.h"
#include "qmf/com/redhat/nodereporter/ArgsNICIdentify_nic.h"

using namespace qpid::management;
using namespace std;

using qpid::management::Manageable;

namespace _qmf = qmf::com::redhat::nodereporter;

extern DBusConnection *dbus_connection;
extern DBusError dbus_error;

ostream &operator<<(ostream& output, const NICWrapper& nic) {
    output << "NIC" << endl;
    output << "Interface Name: " << nic.interfaceName << endl;
    output << "MAC Address: " << nic.macaddr << endl;
    output << "IP Address: " << nic.ipaddr << endl;
    output << "Netmask: " << nic.netmask << endl;
    output << "Broadcast: " << nic.broadcast << endl;
    output << "Bandwidth: " << nic.bandwidth << endl;
    return output;
}

void NICWrapper::setupQMFObject(ManagementAgent *agent, Manageable *parent)
{
    mgmt_object = new _qmf::NIC(agent, this, parent);
    agent->addObject(mgmt_object);
    syncQMFObject();
}

void NICWrapper::cleanupQMFObject(void)
{
    mgmt_object->resourceDestroy();
}

void NICWrapper::syncQMFObject(void)
{
    mgmt_object->set_interface(interfaceName);
    mgmt_object->set_macaddr(macaddr);
    mgmt_object->set_ipaddr(ipaddr);
    mgmt_object->set_netmask(netmask);
    mgmt_object->set_broadcast(broadcast);
    mgmt_object->set_bandwidth(bandwidth);
}

NICWrapper *NICWrapper::getNIC(ManagementAgent *agent, 
		   LibHalContext *hal_ctx,
		   char *nic_handle)
{
    // Used to get the data
    char *macaddr_c;
    char *interface_c;
    int sock, ret;
    struct ifreq ifr;
    struct ethtool_cmd ecmd;

    // The data that we care about
    NICWrapper *nic = NULL;
    string macaddr;
    string interface;
    string ipaddr;
    string netmask;
    string broadcast;
    int bandwidth;
    
    // Grab the MAC Address from libhal
    macaddr_c = libhal_device_get_property_string(hal_ctx,
						nic_handle,
						"net.address",
						&dbus_error);
    // Or throw an exception if we could not find it. No cleanup yet.
    if (!macaddr_c)
        throw runtime_error("Could not get mac address for NIC");

    // Grab the interface name from libhal or return cleanup and fail
    interface_c = libhal_device_get_property_string(hal_ctx,
                          nic_handle,
                          "net.interface",
                          &dbus_error);
    // Or cleanup the macaddr and return NULL.
    if (!interface_c) {
        libhal_free_string(macaddr_c);
        throw runtime_error("Could not get interface name for nic");
    }

    // Open socket for running ioctls for getting rest of data
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock >= 0) {
        // Get the IP Address
        ifr.ifr_addr.sa_family = AF_INET;
        strncpy(ifr.ifr_name, interface_c, IFNAMSIZ - 1);
        cout << interface_c << endl;

        ret = ioctl(sock, SIOCGIFADDR, &ifr);
        if(ret == 0) {
            struct sockaddr_in *addr = (struct sockaddr_in *) &ifr.ifr_addr;
            cout << "ip_address = " << inet_ntoa(addr->sin_addr) << endl;
            ipaddr = inet_ntoa(addr->sin_addr);
        }
        else {
            perror("SIOCGIFADDR");
            ipaddr = "unable to determine";
        }
        // Get the netmask
        ret = ioctl(sock, SIOCGIFNETMASK, &ifr);
        if(ret == 0 && strcmp("255.255.255.255",  
            inet_ntoa(((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr))) {

            struct sockaddr_in *addr = (struct sockaddr_in *) &ifr.ifr_addr;
            cout << "netmask = " << inet_ntoa(addr->sin_addr) << endl;
            netmask = inet_ntoa(addr->sin_addr);
        }
        else {
            perror("SIOCGIFNETMASK");
            netmask = "unable to determine";
        }
        // Get the broadcast address
        ret = ioctl(sock, SIOCGIFBRDADDR, &ifr);
        if(ret == 0) {
            struct sockaddr_in *addr = (struct sockaddr_in *) &ifr.ifr_addr;
            cout << "broadcast = " << inet_ntoa(addr->sin_addr);
            broadcast = inet_ntoa(addr->sin_addr);
        }
        else {
            perror("SIOCGIFBRDADDR");
            broadcast = "unable to determine";
        }
        // Get the bandwidth for this NIC
        ecmd.cmd = ETHTOOL_GSET;
        ifr.ifr_data = (caddr_t)&ecmd;
        bandwidth = 10;

        // TODO: Error checking on this ioctl. For now, assume success
        ret = ioctl(sock, SIOCETHTOOL, &ifr);
        if (1) {

            if (ecmd.supported & SUPPORTED_10000baseT_Full) {
                bandwidth = 10000;
            } else if (ecmd.supported & SUPPORTED_2500baseX_Full) {
                bandwidth = 2500;
            } else if (ecmd.supported & (SUPPORTED_1000baseT_Half |
                                       SUPPORTED_1000baseT_Full)) {
                bandwidth = 1000;
            } else if (ecmd.supported & (SUPPORTED_100baseT_Half |
                                       SUPPORTED_100baseT_Full)) {
                bandwidth = 100;
            } else if (ecmd.supported & (SUPPORTED_10baseT_Half |
                                       SUPPORTED_10baseT_Full)) {
                bandwidth = 10;
            }
        }
        else {
            cout << "Unable to determine link speed, defaulting to 10" << endl;
        }
        // And we're done here
        close(sock);
    }
    else {
        /* Couldn't open socket, so cleanup and fail */
        libhal_free_string(interface_c);
        libhal_free_string(macaddr_c);
        throw runtime_error("Unable to open socket.");
    }
    // We have all the data. Create the NICWrapper instance
    macaddr = macaddr_c;
    interface = interface_c;
    nic = new NICWrapper(interface,
			 macaddr,
			 ipaddr,
			 netmask,
			 broadcast,
			 bandwidth);

    // Free resources and return
    libhal_free_string(interface_c);
    libhal_free_string(macaddr_c);
    return nic;
}

/**
 * void fillNICInfo(vector <NICWrapper*> &nics, 
 *                  ManagementAgent *agent, 
 *                  LibHalContext *hal_ctx)
 *
 * Takes in a vector of NICWrapper object pointers and populates it with
 * NICs found in the system found by querying dbus and making other system
 * calls.
 */
void NICWrapper::fillNICInfo(vector <NICWrapper*> &nics, 
			     ManagementAgent *agent, 
			     LibHalContext *hal_ctx)
{
    char **net_devices;
    int num_results, i;
    net_devices = libhal_find_device_by_capability(hal_ctx, 
                           "net.80203",
                           &num_results, 
                           &dbus_error);
    if (!net_devices)
        throw runtime_error("Error: Couldn't get NIC devices through libhal.");

    cout << "Found " << num_results << " NICs" << endl;
    for (i = 0; i < num_results; i++) {
        NICWrapper *nic;
        char *nic_handle = net_devices[i];
        // If we couldn't read the info for a nic, free the list of devices
        // and throw an error. Any devices added already will be cleaned up
        // by the caller.
        try {
            nic = getNIC(agent, hal_ctx, nic_handle);
        }
        catch (...) {
            libhal_free_string_array(net_devices);
            throw;
        }
        // Add the NIC to our list
        nics.push_back(nic);
    }
    // And we're all done.
    libhal_free_string_array(net_devices);
}

int NICWrapper::identifyNIC(int seconds)
{
    struct ethtool_value edata;
    struct ifreq ifr;
    int sock, ret;

    edata.cmd = ETHTOOL_PHYS_ID;
    edata.data = seconds; // seconds of blink time

    strncpy(ifr.ifr_name, interfaceName.c_str(), IFNAMSIZ - 1);
    ifr.ifr_data = (caddr_t)&edata;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    ret = ioctl(sock, SIOCETHTOOL, &ifr);
    close(sock);

    if (ret != 0)
	ret = errno;

    return ret;
}

Manageable::status_t 
NICWrapper::ManagementMethod(uint32_t methodId, Args& args, string& text)
{
    switch (methodId) {
        case _qmf::NIC::METHOD_IDENTIFY_NIC:
	    _qmf::ArgsNICIdentify_nic& ioArgs = (_qmf::ArgsNICIdentify_nic&) args;
	    int seconds = ioArgs.i_seconds;
	    ioArgs.o_ret = identifyNIC(seconds);
	    return STATUS_OK;
    }

    return STATUS_NOT_IMPLEMENTED;
}
