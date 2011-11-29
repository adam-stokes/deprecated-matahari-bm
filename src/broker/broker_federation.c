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
#include <glib.h>

#include <matahari/dnssrv.h>
#include <matahari/utilities.h>
#include <matahari/logging.h>

#include "broker.h"
#include "broker_os.h"
#include "broker_federation.h"


#define DNS_SRV_PREFIX_TCP "_matahari._tcp."
#define DNS_SRV_PREFIX_TLS "_matahari._tls."

#define IS_DNS_SRV(ADDR, TYPE) (strncmp(ADDR,                                 \
                                        DNS_SRV_PREFIX_##TYPE,                \
                                        strlen(DNS_SRV_PREFIX_##TYPE)) == 0)

static int
broker_add_route(const char *local, const char *remote,
        const char *exchange, const char *route_key, int srclocal,
        int aggregate)
{
    int ret;
    struct mh_qpid_route route[] = {
        { local, remote, exchange, route_key, srclocal, aggregate }
    };

    mh_info("Adding routes for federated broker \"%s\"", remote);
    ret = broker_os_add_qpid_route(route);
    if (ret) {
        mh_err("Failed to route %s => %s", local, remote);
    }
    return ret;
}

static int
broker_federate(const char *local, const char *remote, int route_type)
{
    int ret = broker_os_add_qpid_route_link(local, remote);
    if (ret) {
        mh_err("Failed to link brokers \"%s\" to \"%s\"", local, remote);
        return ret;
    }

    if (route_type) {
        if ((broker_add_route(local, remote, "qmf.default.topic", "direct-agent.#", FALSE, TRUE) ||
             broker_add_route(local, remote, "qmf.default.topic", "console.#", FALSE, TRUE) ||
             broker_add_route(remote, local, "qmf.default.topic", "direct-console.#", TRUE, TRUE) ||
             broker_add_route(remote, local, "qmf.default.topic", "agent.#", TRUE, TRUE)) > 0) {
            return 1;
        }
    } else {
        if ((broker_add_route(remote, local, "amq.direct", NULL, FALSE, FALSE) ||
             broker_add_route(local, remote, "amq.direct", NULL, FALSE, FALSE) ||
             broker_add_route(remote, local, "qmf.default.direct", NULL, FALSE, FALSE) ||
             broker_add_route(local, remote, "qmf.default.direct", NULL, FALSE, FALSE) ||
             broker_add_route(remote, local, "qmf.default.topic", NULL, FALSE, FALSE) ||
             broker_add_route(local, remote, "qmf.default.topic", NULL, FALSE, FALSE)) > 0) {
            return 1;
        }
    }

    return 0;
}

static char *
broker_lookup(const char *query)
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

void
broker_federation_configure(void)
{
    const char *brokers = getenv("FEDERATED_BROKERS");
    const char *proxy_service_port = getenv("VP_G_SERVICE_PORT");
    const char *proxy_service_dir = getenv("VP_G_HOST_DIR");
    char local[16];
    char peers[1024];
    char *peer = NULL;

    if (g_file_test(proxy_service_dir, G_FILE_TEST_IS_DIR)) {
        char remote[16];
        snprintf(remote, sizeof(local), "localhost:%s", proxy_service_port);
        snprintf(local, sizeof(remote), "localhost:%hu", broker_get_port());
        broker_federate(local, remote, TRUE);
    } else if (brokers) {
        snprintf(local, sizeof(local), "localhost:%hu", broker_get_port());

        mh_string_copy(peers, brokers, sizeof(peers));

        while ((peer = strtok(peer ? NULL : peers, ",; ")) != NULL) {
            if (IS_DNS_SRV(peer, TCP) || IS_DNS_SRV(peer, TLS)) {
                peer = broker_lookup(peer);
            }

            if (peer) {
                broker_federate(local, peer, FALSE);
            }
        }

    } else {
        return;
    }
}

