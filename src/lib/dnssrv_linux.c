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

#include <netinet/in.h>
#include <string.h>
#include <glib.h>

#ifdef HAVE_RESOLV_A
#include <arpa/nameser.h>
#include <resolv.h>
#endif

#include "dnssrv_private.h"

struct srv_reply **
srv_lookup(char *service, char *protocol, char *domain)
{
    struct srv_reply **replies = NULL;
#ifdef HAVE_RESOLV_A
    struct srv_reply *reply = NULL;
    char name[MAX_NAME_LEN];
    unsigned char querybuf[MAX_NAME_LEN];
    const unsigned char *buf;
    ns_msg nsh;
    ns_rr rr;
    int i, n, len, size;

    g_snprintf(name, sizeof(name), "_%s._%s.%s", service, protocol, domain);
    if ((size = res_query(name, C_IN, T_SRV, querybuf, sizeof(querybuf))) < 0) {
        return NULL;
    }

    if (ns_initparse(querybuf, size, &nsh) != 0) {
        return NULL;
    }

    n = 0;
    while (ns_parserr(&nsh, ns_s_an, n, &rr) == 0) {
        size = ns_rr_rdlen(rr);
        buf = ns_rr_rdata(rr);
        
        len = 0;
        for (i = 6; i < size && buf[i]; i+= buf[i] + 1) {
            len += buf[i] + 1;
        }

        if (i > size) {
            break;
        }

        reply = g_malloc(sizeof(struct srv_reply) + len);
        memcpy(reply->name, buf+7, len);

        for (i = buf[6]; i < len && buf[7 + i]; i += buf[7 + i] + 1) {
            reply->name[i] = '.';
        }

        if (i > len) {
            g_free(reply);
            break;
        }

        reply->prio = (buf[0] << 8) | buf[1];
        reply->weight = (buf[2] << 8) | buf[3];
        reply->port = (buf[4] << 8) | buf[5];
        n++;
        replies = g_renew(struct srv_reply *, replies, n+1);
        replies[n - 1] = reply;
    }
    if (replies) {
        replies[n] = NULL;
    }
#endif
    return replies;
}

void srv_free(struct srv_reply **srv)
{
    int i;
    if (srv == NULL) {
        return;
    }

    for (i = 0; srv[i]; i++) {
        g_free(srv[i]);
    }
    g_free(srv);
}
    
/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: et
 */

