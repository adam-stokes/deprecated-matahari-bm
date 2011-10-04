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

/**
 * Callback function, doesn't do anything
 */
void f(void *, int)
{
}

class MhApiSysconfigWindowsSuite : public CxxTest::TestSuite
{
 public:
    void testPlaceHolder(void)
    {
        TS_ASSERT( 1 + 1 > 1);
    }
};

#endif
