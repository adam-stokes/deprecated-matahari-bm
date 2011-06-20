/* dnssrv.c - Copyright (c) 2011 Red Hat, Inc.
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

#include <string.h>
#include <errno.h>
#include <glib.h>
#include <stdlib.h>

#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>

#include "matahari/dnssrv.h"

int
mh_srv_lookup(const char *query, char *target)
{
    union {
        HEADER hdr;
        unsigned char buf[NS_PACKETSZ];
    } answer;
    ns_msg nsh;
    ns_rr rr;
    int size, rrnum;

    size = res_query(query,
                     C_IN, 
                     T_SRV,
                     (u_char *)&answer,
                     sizeof(answer));
    if (size > 0) {
        if (ns_initparse(answer.buf, size, &nsh) < 0) {
            goto fail;
        }
    }

    for (rrnum = 0; rrnum < ns_msg_count(nsh, ns_s_an); rrnum++) {
        if (ns_parserr(&nsh, ns_s_an, rrnum, &rr)) {
            goto fail;
        }

        if (ns_rr_type(rr) == T_SRV) {
            char buf[NS_MAXDNAME];
            /* Only care about domain name from rdata
             * First 6 elements in rdata are broken up
             * contain dns information such as type, class
             * ttl, rdlength.
             */
            ns_name_uncompress(ns_msg_base(nsh),
                               ns_msg_end(nsh),
                               ns_rr_rdata(rr)+6,
                               buf,
                               sizeof(buf));
            strcpy(target, buf);
        }
    }
    return 0;
fail:
    return -1;
}
