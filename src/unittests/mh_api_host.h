/*
 * mh_api_host.h: host unittest
 *
 * Copyright (C) 2011 Red Hat Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 *
 * Author: Adam Stokes <astokes@fedoraproject.org>
 */


#ifndef __MH_API_HOST_UNITTEST_H
#define __MH_API_HOST_UNITTEST_H
#include <iostream>
#include <sstream>
#include <cstring>
#include <utility>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <sys/stat.h>
#include <dirent.h>
#include <pcre.h>
#include <cxxtest/TestSuite.h>

extern "C" {
#include "matahari/host.h"
#include "mh_test_utilities.h"
#include <sigar.h>
#include <sigar_format.h>
#include <glib.h>
};

#define OVECCOUNT 30

using namespace std;

class MhApiHostSuite : public CxxTest::TestSuite
{
 public:
    std::stringstream infomsg;
    std::stringstream errmsg;

    void testHostName(void)
    {
        infomsg << "Verify " << mh_host_get_hostname() << " format";
        TS_TRACE(infomsg.str());
        /* http://stackoverflow.com/questions/1418423/the-hostname-regex */
        TS_ASSERT((mh_test_is_match("^(?=.{1,255}$)[0-9A-Za-z](?:(?:[0-9A-Za-z]|\\b-){0,61}[0-9A-Za-z])?(?:\\.[0-9A-Za-z](?:(?:[0-9A-Za-z]|\\b-){0,61}[0-9A-Za-z])?)*\\.?$",
                        mh_host_get_hostname())) >= 0);
        infomsg.str("");
    }

    void testUuid(void)
    {
        const char *lifetime = NULL;
        infomsg << "Verify " << mh_host_get_uuid(lifetime) << " exists";
        TS_TRACE(infomsg.str());
        TS_ASSERT((mh_test_is_match("^[0-9a-zA-Z]+$", mh_host_get_uuid(lifetime))) >= 0);
        infomsg.str("");
    }

    void testOperatingSystem(void)
    {
        infomsg << "Verify OS : " << mh_host_get_operating_system();
        TS_TRACE(infomsg.str());
        TS_ASSERT((mh_test_is_match("^Linux.*|Windows.*|Solaris.*$",
                            mh_host_get_operating_system())) >= 0);
        infomsg.str("");
    }

    void testArchitecture(void)
    {
        infomsg << "Verify architecture property: " \
                << mh_host_get_architecture();
        TS_TRACE(infomsg.str());
        TS_ASSERT((mh_test_is_match("^[0-9a-zA-Z_]+$",
                            mh_host_get_architecture())) >= 0);
        infomsg.str("");
    }

    void testCpuModel(void)
    {
        infomsg << "Verify cpu model: " << mh_host_get_cpu_model();
        TS_TRACE(infomsg.str());
        TS_ASSERT((mh_test_is_match("\\sCPU\\s", mh_host_get_cpu_model())) >= 0);
        infomsg.str("");
    }

    void testCpuFlags(void)
    {
        infomsg << "Verify cpu flags: " << mh_host_get_cpu_flags();
        TS_TRACE(infomsg.str());
        TS_ASSERT((mh_test_is_match("\\d+", mh_host_get_cpu_flags())) >= 0);
        infomsg.str("");
    }

    void testCpuCount(void)
    {
        infomsg << "Verify cpu count: " << mh_host_get_cpu_count();
        TS_TRACE(infomsg.str());
        TS_ASSERT((mh_host_get_cpu_count()) > 0);
        infomsg.str("");
    }

    void testCpuNumberOfCores(void)
    {
        infomsg << "Verify cpu num of cores: " << mh_host_get_cpu_number_of_cores();
        TS_TRACE(infomsg.str());
        TS_ASSERT((mh_host_get_cpu_number_of_cores()) > 0);
        infomsg.str("");
    }

    void testCpuWordSize(void)
    {
        infomsg << "Verify cpu wordsize: " << mh_host_get_cpu_wordsize();
        TS_TRACE(infomsg.str());
        TS_ASSERT((mh_host_get_cpu_wordsize()) > 0);
        infomsg.str("");
    }

    void testMemory(void)
    {
        infomsg << "Verify memory exist: " << mh_host_get_memory();
        TS_TRACE(infomsg.str());
        TS_ASSERT((mh_host_get_memory()) > 0);
        infomsg.str("");
    }

    void testMemoryFree(void)
    {
        infomsg << "Verify memory free: " << mh_host_get_mem_free();
        TS_TRACE(infomsg.str());
        TS_ASSERT((mh_host_get_mem_free()) > 0);
        infomsg.str("");
    }

    void testSwap(void)
    {
        infomsg << "Verify swap exists: " << mh_host_get_swap();
        TS_TRACE(infomsg.str());
        TS_ASSERT((mh_host_get_swap()) > 0);
        infomsg.str("");
    }

    void testSwapFree(void)
    {
        infomsg << "Verify swap free: " << mh_host_get_swap_free();
        TS_TRACE(infomsg.str());
        TS_ASSERT((mh_host_get_swap_free()) > 0);
        infomsg.str("");
    }

};

#endif
