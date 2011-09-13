/* Copyright (C) 2011 Red Hat, Inc.
 * Written by Zane Bitter <zbitter@redhat.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <matahari/dnssrv.h>
#include <matahari/utilities.h>
#include <matahari/logging.h>

#include "broker.h"
#include "broker_os.h"
#include "broker_federation.h"


#define DNS_SRV_PREFIX "_matahari._tcp."

#define ROUTE_LIST(LOCAL, REMOTE) {          \
    {REMOTE, LOCAL, "amq.direct"},           \
    {LOCAL, REMOTE, "amq.direct"},           \
    {REMOTE, LOCAL, "qmf.default.direct"},   \
    {LOCAL, REMOTE, "qmf.default.direct"},   \
    {REMOTE, LOCAL, "qmf.default.topic"},    \
    {LOCAL, REMOTE, "qmf.default.topic"}     \
}


static int broker_federate(const char *local, const char *remote)
{
    int i;
    struct mh_qpid_route routes[] = ROUTE_LIST(local, remote);

    mh_trace("Adding routes for federated broker \"%s\"", remote);

    for (i = 0; i < DIMOF(routes); i++) {
        int ret = broker_os_add_qpid_route(&routes[i]);
        if (ret) {
            mh_err("Failed to add federated broker \"%s\"", remote);
            return ret;
        }
    }

    return 0;
}

static char *broker_lookup(const char *query)
{
#ifdef HAVE_RESOLV_H
    static char broker[1025 + 6];
    char host[1024];
    uint16_t port;
    mh_trace("Looking up DNS SRV record for \"%s\"", query);
    int res;

    res = mh_dnssrv_lookup_single(query, host, sizeof(host), &port);

    if (!res) {
        snprintf(broker, sizeof(broker), "%s:%hu", host, port);
        return broker;
    }

    mh_warn("Failed to find DNS SRV record for \"%s\"", query);
#else
    mh_warn("DNS SRV lookups are not supported");
#endif

    return NULL;
}

void broker_federation_configure(void)
{
    const char *brokers = getenv("FEDERATED_BROKERS");
    char local[16];
    char peers[1024];
    char *p = NULL;
    char *peer;

    if (!brokers) {
        return;
    }

    snprintf(local, sizeof(local), "localhost:%hu", broker_get_port());

    strncpy(peers, brokers, sizeof(peers) - 1);
    peers[sizeof(peers) - 1] = '\0';

    while ((peer = strtok_r(p ? NULL : peers, ",; ", &p)) != NULL) {
        if (strncmp(peer, DNS_SRV_PREFIX, strlen(DNS_SRV_PREFIX)) == 0) {
            peer = broker_lookup(peer);
        }

        if (peer) {
            broker_federate(local, peer);
        }
    }
}

