#ifndef __MH_API_NETWORK_UNITTEST_H
#define __MH_API_NETWORK_UNITTEST_H
#include <iostream>
#include <cstring>
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
#include <glib.h>

#include "matahari/sysconfig.h"
#include "matahari/sysconfig_internal.h"
#include "mh_test_utilities.h"
};

using namespace std;

class MhApiSysconfigSuite : public CxxTest::TestSuite
{
 public:

    void testIsConfigured(void)
    {
        const char key[] = "org.matahariproject.test.unittest"; // Unimportant key defined
        char *key_res;

        mh_sysconfig_keys_dir_set("/tmp/sysconfig-keys/");

        TS_ASSERT((mh_sysconfig_set_configured(key, "OK")) == TRUE);
        TS_ASSERT(((key_res = mh_sysconfig_is_configured(key))) != NULL);
        TS_ASSERT(!strcmp("OK", key_res));

        free(key_res);
    }
};

#endif
