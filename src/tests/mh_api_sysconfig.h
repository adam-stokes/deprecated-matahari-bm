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
#include "matahari/utilities.h"
#include "mh_test_utilities.h"
};

using namespace std;

class MhApiSysconfigSuite : public CxxTest::TestSuite
{
 public:

    void testIsConfigured(void)
    {
        const char key[] = "org.matahariproject.test.unittest"; // Unimportant key defined
        const char *invalid_keys[3] = {"../etc/passwd",
                                       "./../etc/passwd#",
                                       "HAPPY#HAMMY,@"};
        char *key_res;

        mh_sysconfig_keys_dir_set("/tmp/matahari-sysconfig-keys/");

        TS_ASSERT((mh_sysconfig_set_configured(key, "OK")) == MH_RES_SUCCESS);
        TS_ASSERT(((key_res = mh_sysconfig_is_configured(key))) != NULL);
        TS_ASSERT(!strcmp("OK", key_res));

        free(key_res);
        for (int i = 0; i < DIMOF(invalid_keys); i++) {
            TS_ASSERT((mh_sysconfig_set_configured(invalid_keys[i], "OK")) == MH_RES_INVALID_ARGS);
        }
    }
};

#endif
