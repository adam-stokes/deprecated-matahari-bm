/*
 * mh_hs.h: dns srv high availability test 
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


#ifndef __MH_HSA_UNITTEST_H
#define __MH_HSA_UNITTEST_H

#include <string.h>
#include <stdlib.h>
#include <cxxtest/TestSuite.h>

extern "C" {
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "mh_test_utilities.h"
#include "matahari/dnssrv.h"
};

using namespace std;

class MhHsaSuite : public CxxTest::TestSuite
{
 public:
     std::stringstream infomsg;

     void testSrvLookup(void)
     {
         char *target;
         const char *host = "_matahari._tcp.matahariproject.org";

         target = mh_dnssrv_lookup(host);
         TS_ASSERT(target != NULL);
         TS_ASSERT((mh_test_is_match("^www\\.matahariproject\\.org$",
                                     target)) >= 0);
         free(target);
     }
};

#endif
