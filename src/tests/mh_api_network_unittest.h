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
    void setUp (void)
    {
        // will be setup
    }

    void tearDown (void)
    {
        // teardown
        
    }

    void testListNetworkDevices(void)
    {
        GList *plist = NULL;
        GList *interface_list = NULL;
        
        interface_list = mh_network_get_interfaces();
        TS_ASSERT (!interface_list.empty());
    }
};

#endif
