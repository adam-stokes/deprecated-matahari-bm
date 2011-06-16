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
#include <glib.h>
#include <stdlib.h>

#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>

#include "dnssrv_private.h"

int
srv_lookup(char *query, char *target)
{
    union {
        HEADER hdr;
        unsigned char buf[MAX_NAME_LEN];
    } answer;
    ns_msg nsh;
    ns_rr rr;
    unsigned char buf[MAX_NAME_LEN], *p;
    int t, size;

    size = res_query(query, C_IN, T_SRV, (unsigned char*)&answer, sizeof(answer));

    ns_initparse(answer.buf, size, &nsh);
    if (ns_msg_count(nsh, ns_s_an) == 0) {
        return -1;
    }

    if (ns_parserr(&nsh, ns_s_an, 0, &rr) < 0) {
        return -1;
    }

    size = ns_rr_rdlen(rr);
    memcpy(buf, ns_rr_rdata(rr), size);
    p = buf;
    p += 6;
    while (*p > 0) {
        t = *p;
        *p = '.';
        p += t+1;
    }
    p = buf + 7;
    strcpy(target, p); 
    return 0;
}
