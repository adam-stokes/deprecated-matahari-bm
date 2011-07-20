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
#include "matahari/sysconfig.h"
#include <glib.h>
#include "mh_test_utilities.h"
};

using namespace std;

class MhApiSysconfigSuite : public CxxTest::TestSuite
{
 public:

    void testIsConfigured(void)
    {
        conf_t cf = {};
        cf.uri = "http://matahariproject.org/atom.xml";
        cf.flags = 0;
        cf.scheme = "puppet";
        TS_ASSERT((mh_sysconfig_run_uri(cf.uri, cf.flags, cf.scheme)) != -1);
    }
};

#endif
