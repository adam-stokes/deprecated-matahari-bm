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
};

using namespace std;

class MhApiNetworkSuite : public CxxTest::TestSuite
{
 public:
    GList *plist;
    GList *interface_list;
    sigar_net_interface_config_t *ifconfig;
    vector<char*> iface_names;
    
    void setUp (void)
    {
        interface_list = mh_network_get_interfaces();
        for (plist = g_list_first(interface_list); plist;
              plist = g_list_next(plist)) {
            ifconfig = (sigar_net_interface_config_t *)plist->data;
            iface_names.push_back(ifconfig->name);
        }
    }
    
    void tearDown (void)
    {
        g_list_free(interface_list);
    }
    
    void testListNetworkDevices(void)
    {
        unsigned int i;
        
        for(i=0; i<iface_names.size(); i++) {
            TS_ASSERT(iface_names[i]);
        }
    }
};

#endif
