/* dnssrv_windows.c - Copyright (c) 2011 Red Hat, Inc.
 * Written by Adam Stokes <astokes@fedoraproject.org>
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

// mingw doesn't include these
#ifndef DNS_TYPE_SRV
# define DNS_TYPE_SRV 33
#endif

#ifndef NS_MAXDNAME
# define NS_MAXDNAME 1025
#endif

/**
 * Domain lookup providing a Matahari broker
 *
 * \param[in] srv record query i.e. "_matahari._tcp.matahariproject.org
 * \param[in] set buffer to hold domain retrieved
 * \param[in] set buffer length 
 *
 * \return 0 or greater for successful match
 */

int
mh_srv_lookup(const char *query, char *target, size_t len)
{
    PDNS_RECORD rr, record;
    WCHAR query_wstr[len];

   
    MultiByteToWideChar(CP_UTF8, 0, query, len, query_wstr, len); 
    if (DnsQuery(query_wstr, DNS_TYPE_SRV, 
                DNS_QUERY_STANDARD, NULL, 
                &rr, NULL) == ERROR_SUCCESS) {


        record = rr;
        do {
            if (record->wType == DNS_TYPE_SRV) {
                WideCharToMultiByte(CP_UTF8, 0, query_wstr, len, target, len, NULL, NULL);
                if (len > 0) {
                    target[len - 1] = '\0';
                }
            }
            record = record->pNext;
        } while (rr != NULL);
        DnsRecordListFree(rr, DnsFreeRecordList);
        return 0;
    } else {
        return -1;
    }
}
