/* dnssrv.c - Copyright (c) 2011 Red Hat, Inc.
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

#include <string.h>
#include <errno.h>
#include <glib.h>
#include <stdlib.h>
#include <sys/types.h>

#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <limits.h>

#include "dnssrv_private.h"
#include "matahari/dnssrv.h"
#include "matahari/dnssrv_internal.h"

GList *
mh_os_dnssrv_lookup(const char *query)
{
    union {
        HEADER hdr;
        unsigned char buf[NS_PACKETSZ];
    } answer;
    ns_msg nsh = { NULL, };
    ns_rr rr;
    int size, rrnum;
    GList *records = NULL;

    size = res_query(query, C_IN, T_SRV, (u_char *) &answer, sizeof(answer));

    if (size > 0) {
        if (ns_initparse(answer.buf, size, &nsh) < 0) {
            return NULL;
        }
    }

    for (rrnum = 0; rrnum < ns_msg_count(nsh, ns_s_an); rrnum++) {
        char host[NS_MAXDNAME];
        uint16_t port, priority, weight;
        uint16_t *data;

        if (ns_parserr(&nsh, ns_s_an, rrnum, &rr)) {
            goto error_cleanup;
        }

        if (ns_rr_type(rr) != T_SRV) {
            continue;
        }

        data = (uint16_t *) ns_rr_rdata(rr);
        priority = ntohs(data[0]);
        weight = ntohs(data[1]);
        port = ntohs(data[2]);

        ns_name_uncompress(ns_msg_base(nsh), ns_msg_end(nsh),
                           ns_rr_rdata(rr) + 6, host, sizeof(host));

        records = mh_dnssrv_add_record(records, host, port, priority, weight);
    }

    return records;

error_cleanup:
    g_list_free_full(records, mh_dnssrv_record_free);

    return NULL;
}
