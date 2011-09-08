/* dnssrv_windows.c - Copyright (c) 2011 Red Hat, Inc.
 * Written by Adam Stokes <astokes@fedoraproject.org>
 * Written by Russell Bryant <rbryant@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "matahari/dnssrv.h"

#include <windows.h>
#include <winsock.h>
#include <windns.h>
#include <ws2tcpip.h>

// mingw doesn't include these
#ifndef DNS_TYPE_SRV
# define DNS_TYPE_SRV 33
#endif

GList *
mh_os_dnssrv_lookup(const char *query)
{
    PDNS_RECORD rr, record;
    int len = strlen(query) + 1;
    WCHAR query_wstr[len];
    GList *records = NULL;

    MultiByteToWideChar(CP_UTF8, 0, query, len, query_wstr, len);
    if (DnsQuery(query_wstr, DNS_TYPE_SRV,
                DNS_QUERY_STANDARD, NULL,
                &rr, NULL) != ERROR_SUCCESS) {
        return NULL;
    }

    for (record = rr; record; record = record->pNext) {
        char host[NI_MAXHOST];
        uint16_t port;
        uint16_t priority;
        uint16_t weight;

        if (record->wType != DNS_TYPE_SRV) {
            continue;
        }

        /* record->Data.Srv.wPort */
        len = 1 + wcslen(record->Data.Srv.pNameTarget);
        WideCharToMultiByte(CP_UTF8, 0, record->Data.Srv.pNameTarget, len, host, sizeof(host), NULL, NULL);

        port = (uint16_t) record->Data.Srv.wPort;
        priority = (uint16_t) record->Data.Srv.wPriority;
        weight = (uint16_t) record->Data.Srv.wWeight;

        records = mh_dnssrv_add_record(records, host, port, priority, weight);
    }

    DnsRecordListFree(rr, DnsFreeRecordList);

    return records;
}
