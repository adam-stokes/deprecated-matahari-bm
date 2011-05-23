#ifndef __MH_API_NETWORK_UNITTEST_H
#define __MH_API_NETWORK_UNITTEST_H
#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <sys/stat.h>
#include <dirent.h>
#include <cxxtest/TestSuite.h>

extern "C" {
#include "matahari/network.h"
#include "matahari/host.h"
#include <sigar.h>
#include <sigar_format.h>
#include <glib.h>
#include "mh_test_utilities.h"
};

using namespace std;

class MhApiNetworkSuite : public CxxTest::TestSuite
{
 public:
    GList *plist;
    GList *interface_list;
    sigar_net_interface_config_t *ifconfig;
    vector<char*> iface_names;

    void init()
    {
        interface_list = mh_network_get_interfaces();
        for (plist = g_list_first(interface_list); plist;
            plist = g_list_next(plist)) {
            ifconfig = (sigar_net_interface_config_t *)plist->data;
            iface_names.push_back(ifconfig->name);
        }
       g_list_free(interface_list);
    }

    void testListNetworkDevices(void)
    {
        unsigned int i;

        /* only initialize on first test in suite */
        init();

        for(i=0; i<iface_names.size(); i++) {
            TS_TRACE(iface_names[i]);
            TS_ASSERT(iface_names[i]);
        }
    }

    void testGetNetworkIP(void)
    {
        unsigned int i;
        const char *ip = NULL;
        for(i=0; i<iface_names.size(); i++) {
            ip = mh_network_get_ip_address(iface_names[i]);
            TS_TRACE(ip);
            TS_ASSERT((is_match("^\\d+\\.\\d+\\.\\d+\\.\\d+",
                                ip)) >= 0);
        }
    }

    void testGetNetworkMAC(void)
    {
        unsigned int i;
        const char *mac = NULL;
        for(i=0; i<iface_names.size(); i++) {
            mac = mh_network_get_mac_address(iface_names[i]);
            TS_TRACE(mac);
            TS_ASSERT((is_match("^([0-9a-fA-F]{2}([:-]|$)){6}$",
                                mac)) >= 0);
        }
    }
};

#endif
