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
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include "mh_test_utilities.h"
#include "matahari/dnssrv.h"
#include "matahari/dnssrv_internal.h"
};

using namespace std;

class MhHsaSuite : public CxxTest::TestSuite
{
public:
    void testSrvLookupSingle(void)
    {
        char host[256];
        uint16_t port;
        const char query[] = "_matahari._tcp.matahariproject.org";
        int res;

        res = mh_dnssrv_lookup_single(query, host, sizeof(host), &port);
        TS_ASSERT(res == 0);
        TS_ASSERT(port == 49000);
        TS_ASSERT((mh_test_is_match("^www\\.matahariproject\\.org$",
                                    host)) >= 0);
    }

    void testSrvLookup(void)
    {
        const char query[] = "_matahari._tcp.matahariproject.org";
        GList *records;
        struct mh_dnssrv_record *record;

        records = mh_dnssrv_lookup(query);

        record = (struct mh_dnssrv_record *) g_list_nth_data(records, 0);

        TS_ASSERT(mh_dnssrv_record_get_port(record) == 49000);
        TS_ASSERT((mh_test_is_match("^www\\.matahariproject\\.org$",
                                    mh_dnssrv_record_get_host(record))) >= 0);

        g_list_free_full(records, mh_dnssrv_record_free);
    }

    void testSrvPrioritySort(void)
    {
        unsigned int i;
        GList *records_out = NULL;
        struct {
            const char *host;
            uint16_t port;
            uint16_t priority;
            uint16_t weight;
        } records[] = {
            { "a.example.com", 49000, 10, 0 },
            { "b.example.com", 49000, 5, 0 },
            { "c.example.com", 49000, 20, 0 },
            { "d.example.com", 49000, 2, 0 },
            { "e.example.com", 49000, 50, 0 },
            { NULL, }
        };
        struct mh_dnssrv_record *record0;
        struct mh_dnssrv_record *record1;
        struct mh_dnssrv_record *record2;
        struct mh_dnssrv_record *record3;
        struct mh_dnssrv_record *record4;

        for (i = 0; records[i].host ; i++) {
            records_out = mh_dnssrv_add_record(records_out, records[i].host,
                                               records[i].port, records[i].priority,
                                               records[i].weight);
        }

        records_out = mh_dnssrv_records_sort(records_out);

        record0 = (struct mh_dnssrv_record *) g_list_nth_data(records_out, 0);
        record1 = (struct mh_dnssrv_record *) g_list_nth_data(records_out, 1);
        record2 = (struct mh_dnssrv_record *) g_list_nth_data(records_out, 2);
        record3 = (struct mh_dnssrv_record *) g_list_nth_data(records_out, 3);
        record4 = (struct mh_dnssrv_record *) g_list_nth_data(records_out, 4);

        TS_ASSERT(mh_dnssrv_record_get_priority(record0) == 2);
        TS_ASSERT(mh_dnssrv_record_get_priority(record1) == 5);
        TS_ASSERT(mh_dnssrv_record_get_priority(record2) == 10);
        TS_ASSERT(mh_dnssrv_record_get_priority(record3) == 20);
        TS_ASSERT(mh_dnssrv_record_get_priority(record4) == 50);
    }

    void testSrvWeightSort(void)
    {
        unsigned int i;
        GList *records_out = NULL;
        struct {
            const char *host;
            uint16_t port;
            uint16_t priority;
            uint16_t weight;
        } records[] = {
            { "a1.example.com", 49000, 10, 10 },
            { "a2.example.com", 49000, 10, 0 },
            { "a3.example.com", 49000, 10, 20 },
            { "a4.example.com", 49000, 10, 50 },
            { "a5.example.com", 49000, 10, 40 },
            { "b.example.com", 49000, 5, 0 },
            { "c.example.com", 49000, 20, 0 },
            { "d.example.com", 49000, 2, 0 },
            { "e.example.com", 49000, 50, 0 },
            { NULL, }
        };
        struct mh_dnssrv_record *record0;
        struct mh_dnssrv_record *record1;
        struct mh_dnssrv_record *record2;
        struct mh_dnssrv_record *record3;
        struct mh_dnssrv_record *record4;
        struct mh_dnssrv_record *record5;
        struct mh_dnssrv_record *record6;
        struct mh_dnssrv_record *record7;
        struct mh_dnssrv_record *record8;
        std::stringstream tracestr;

        for (i = 0; records[i].host ; i++) {
            records_out = mh_dnssrv_add_record(records_out, records[i].host,
                                               records[i].port, records[i].priority,
                                               records[i].weight);
        }

        records_out = mh_dnssrv_records_sort(records_out);

        record0 = (struct mh_dnssrv_record *) g_list_nth_data(records_out, 0);
        record1 = (struct mh_dnssrv_record *) g_list_nth_data(records_out, 1);
        record2 = (struct mh_dnssrv_record *) g_list_nth_data(records_out, 2);
        record3 = (struct mh_dnssrv_record *) g_list_nth_data(records_out, 3);
        record4 = (struct mh_dnssrv_record *) g_list_nth_data(records_out, 4);
        record5 = (struct mh_dnssrv_record *) g_list_nth_data(records_out, 5);
        record6 = (struct mh_dnssrv_record *) g_list_nth_data(records_out, 6);
        record7 = (struct mh_dnssrv_record *) g_list_nth_data(records_out, 7);
        record8 = (struct mh_dnssrv_record *) g_list_nth_data(records_out, 8);

        TS_ASSERT(mh_dnssrv_record_get_priority(record0) == 2);
        TS_ASSERT(mh_dnssrv_record_get_priority(record1) == 5);
        TS_ASSERT(mh_dnssrv_record_get_priority(record2) == 10);
        tracestr << "Record 2: weight '" << mh_dnssrv_record_get_weight(record2) << "'";
        TS_TRACE(tracestr.str());
        tracestr.str("");
        TS_ASSERT(mh_dnssrv_record_get_priority(record3) == 10);
        tracestr << "Record 3: weight '" << mh_dnssrv_record_get_weight(record3) << "'";
        TS_TRACE(tracestr.str());
        tracestr.str("");
        TS_ASSERT(mh_dnssrv_record_get_priority(record4) == 10);
        tracestr << "Record 4: weight '" << mh_dnssrv_record_get_weight(record4) << "'";
        TS_TRACE(tracestr.str());
        tracestr.str("");
        TS_ASSERT(mh_dnssrv_record_get_priority(record5) == 10);
        tracestr << "Record 5: weight '" << mh_dnssrv_record_get_weight(record5) << "'";
        TS_TRACE(tracestr.str());
        tracestr.str("");
        TS_ASSERT(mh_dnssrv_record_get_priority(record6) == 10);
        tracestr << "Record 6: weight '" << mh_dnssrv_record_get_weight(record6) << "'";
        TS_TRACE(tracestr.str());
        tracestr.str("");
        TS_ASSERT(mh_dnssrv_record_get_priority(record7) == 20);
        TS_ASSERT(mh_dnssrv_record_get_priority(record8) == 50);
    }

};

#endif
