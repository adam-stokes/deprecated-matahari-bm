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
        const char *uri = "http://matahariproject.org/atom.xml"; // Test if download succeeds
        const char flags = 0;
        const char key[] = "org.matahariproject.test.unittest"; // Unimportant key defined

        mh_sysconfig_keys_dir_set("/tmp/");

        TS_ASSERT((mh_sysconfig_set_configured(key, "OK")) == TRUE);
        TS_ASSERT((mh_sysconfig_is_configured(key)) != NULL);
        TS_ASSERT((mh_sysconfig_run_uri(uri, flags, "puppet", key)) != -1);
    }
};

#endif
