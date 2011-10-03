#ifndef __MH_API_NETWORK_UNITTEST_H
#define __MH_API_NETWORK_UNITTEST_H
#include <iostream>
#include <string>
#include <sstream>
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
    std::stringstream infomsg;

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
        /* only initialize on first test in suite */
        init();
        
        for(std::vector<char *>::iterator it = iface_names.begin(); 
            it != iface_names.end(); ++it) {
            infomsg << "Checking for available interfaces: " << *it;
            TS_TRACE(infomsg.str());
            TS_ASSERT(*it);
            infomsg.str("");
        }
    }

    void testGetNetworkIP(void)
    {
        const char *ip = NULL;
        char buf[64] = "blah";

        /*
         * Ensure that buf gets initialized even if a bogus interface
         * name is specified.
         */
        ip = mh_network_get_ip_address("fakeInterface", buf, sizeof(buf));
        TS_ASSERT(*ip == '\0');

        /*
         * Now verify we can get a valid IP address for each interface
         * on this system.
         */
        for(std::vector<char *>::iterator it = iface_names.begin(); 
            it != iface_names.end(); ++it) {
          ip = mh_network_get_ip_address(*it, buf, sizeof(buf));
          infomsg << "Verify IP address format: " << ip;
          TS_TRACE(infomsg.str());
          TS_ASSERT((mh_test_is_match("^\\d+\\.\\d+\\.\\d+\\.\\d+",
                                      ip)) >= 0);
          infomsg.str("");
        }

    }

    void testGetNetworkMAC(void)
    {
        const char *mac = NULL;
        char buf[64];
        for(std::vector<char *>::iterator it = iface_names.begin(); 
            it != iface_names.end(); ++it) {
            mac = mh_network_get_mac_address(*it, buf, sizeof(buf));
            infomsg << "Verify MAC address format: " << mac;
            TS_TRACE(infomsg.str());
            TS_ASSERT((mh_test_is_match("^([0-9a-fA-F]{2}([:-]|$)){6}$",
                                        mac)) >= 0);
            infomsg.str("");
        }
    }
};

#endif
