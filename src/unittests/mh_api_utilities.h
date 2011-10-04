#ifndef __MH_API_UTILITIES_UNITTEST_H
#define __MH_API_UTILITIES_UNITTEST_H

#include <cstring>
#include <cxxtest/TestSuite.h>

extern "C" {
#include "matahari/utilities.h"
};

using namespace std;

class MhApiUtilitiesSuite : public CxxTest::TestSuite
{
public:
    void testStrlenZero(void)
    {
        TS_ASSERT(mh_strlen_zero("foo") == 0);
        TS_ASSERT(mh_strlen_zero("") != 0);
        TS_ASSERT(mh_strlen_zero(NULL) != 0);
    }

    void testCopyString(void)
    {
        char out[8] = "abc";

        mh_string_copy(out, "x", 0);
        TS_ASSERT(strcmp(out, "abc") == 0);

        mh_string_copy(out, "", sizeof(out));
        TS_ASSERT(out[0] == '\0');
        TS_ASSERT(out[1] == 'b');

        mh_string_copy(out, "z", sizeof(out));
        TS_ASSERT(out[0] == 'z');
        TS_ASSERT(out[1] == '\0');
        TS_ASSERT(out[2] == 'c');

        mh_string_copy(out, "1234567890", sizeof(out));
        TS_ASSERT(strcmp(out, "1234567") == 0);
    }
};

#endif
