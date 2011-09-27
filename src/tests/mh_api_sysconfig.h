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

/**
 * Result from run_string and run_uri functions is in the form "query = result\n"
 * This function will extract the "result"
 *
 * \note The return of this routine must be freed with free()
 */
char *strip(const char *result)
{
    char *stripped;
   // strip the "query = " part
    stripped = strdup(strstr(result, " = ") + 3);
    // and the line end
    stripped[strlen(stripped) - 1] = '\0';
    return stripped;
}

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

    /**
     * This test compares result of mh_sysconfig_query, mh_sysconfig_run_string
     * and mh_sysconfig_run_uri commands.
     */
    void testAugeasQuery(void)
    {
        const char key[] = "org.matahariproject.test.unittest.augeas";
        const char query[] = "/files/etc/fstab/1/spec";
        const char command[] = "get /files/etc/fstab/1/spec";
        char tmp_file[] = "matahari-sysconfig-test_XXXXXX";
        char *query_result, *run_string_result, *run_string_result_stripped;
        char *run_uri_result, *run_uri_result_stripped;
        char *uri, *abs_path;
        int fd = mkstemp(tmp_file);
        TS_ASSERT(fd >= 0);

        // test query function
        query_result = mh_sysconfig_query(query, 0, "augeas");
        TS_ASSERT(query_result != NULL);
        TS_ASSERT(strlen(query_result) > 0);

        // test run_string function
        TS_ASSERT(mh_sysconfig_run_string(command, MH_SYSCONFIG_FLAG_FORCE, "augeas", key, f, NULL) == MH_RES_SUCCESS);
        run_string_result = mh_sysconfig_is_configured(key);
        TS_ASSERT(run_string_result != NULL);

        run_string_result_stripped = strip(run_string_result);
        free(run_string_result);

        // test run_uri function
        write(fd, command, strlen(command));
        write(fd, "\n", 1);
        close(fd);

        abs_path = realpath(tmp_file, NULL);
        asprintf(&uri, "file://%s", abs_path);
        TS_ASSERT(mh_sysconfig_run_uri(uri, MH_SYSCONFIG_FLAG_FORCE, "augeas", key, f, NULL) == MH_RES_SUCCESS);
        run_uri_result = mh_sysconfig_is_configured(key);
        TS_ASSERT(run_uri_result != NULL);

        run_uri_result_stripped = strip(run_uri_result);
        free(run_uri_result);
        unlink(tmp_file);

        // compare the results of all functions above
        TS_ASSERT(strcmp(query_result, run_string_result_stripped) == 0);
        TS_ASSERT(strcmp(query_result, run_uri_result_stripped) == 0);

        free(query_result);
        free(run_string_result_stripped);
        free(run_uri_result_stripped);
        free(abs_path);
        free(uri);
    }
};

#endif
