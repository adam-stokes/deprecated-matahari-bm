/* network.c - Copyright (c) 2010 Red Hat, Inc.
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

#ifndef WIN32
# include "config.h"
#endif

#include "matahari/network.h"
#include "network_private.h"
#include "matahari/logging.h"
#include <sigar.h>
#include <sigar_format.h>
#include <stdint.h>

MH_TRACE_INIT_DATA(mh_network);

struct mh_network_interface {
    sigar_net_interface_config_t ifconfig;
};

const char *
mh_network_interface_get_name(const struct mh_network_interface *iface)
{
    return iface->ifconfig.name;
}

uint64_t
mh_network_interface_get_flags(const struct mh_network_interface *iface)
{
    uint64_t flags = 0;

    if (iface->ifconfig.flags & SIGAR_IFF_UP) {
        flags = MH_NETWORK_IF_UP;
    } else {
        flags = MH_NETWORK_IF_DOWN;
    }

    return flags;
}

void
mh_network_interface_destroy(gpointer data)
{
    g_free(data);
}

static GList *
query_interface_table(void)
{
    int status;
    uint32_t lpc = 0;
    GList *interfaces = NULL;
    sigar_t* sigar;
    sigar_net_interface_list_t iflist;

    sigar_open(&sigar);

    status = sigar_net_interface_list_get(sigar, &iflist);
    if (status != SIGAR_OK) {
        goto return_cleanup;
    }

    for (lpc = 0; lpc < iflist.number; lpc++) {
        struct mh_network_interface *iface;

        iface = g_malloc0(sizeof(*iface));

        status = sigar_net_interface_config_get(sigar, iflist.data[lpc],
                                                &iface->ifconfig);

        if (status != SIGAR_OK) {
            g_free(iface);
            continue;
        }

        interfaces = g_list_prepend(interfaces, iface);
    }

    sigar_net_interface_list_destroy(sigar, &iflist);

return_cleanup:
    sigar_close(sigar);

    return interfaces;
}

GList *
mh_network_get_interfaces(void)
{
    return query_interface_table();
}

void
mh_network_start(const char *iface)
{
    network_os_start(iface);
}

void
mh_network_stop(const char *iface)
{
    network_os_stop(iface);
}

void
mh_network_restart(const char *iface)
{
    network_os_stop(iface);
    network_os_start(iface);
}

int
mh_network_status(const char *iface, uint64_t *flags)
{
    GList *list = NULL;
    GList *plist;
    int res = 1;

    list = mh_network_get_interfaces();
    for (plist = g_list_first(list); plist; plist = g_list_next(plist)) {
        struct mh_network_interface *mh_iface = plist->data;

        if ((g_str_equal(mh_network_interface_get_name(mh_iface), iface)) == TRUE) {
            *flags = mh_network_interface_get_flags(mh_iface);
            res = 0;
        }
    }
    g_list_free_full(list, mh_network_interface_destroy);

    return res;
}

const char *
mh_network_get_ip_address(const char *iface, char *buf, size_t len)
{
    GList *list = NULL;
    GList *plist;
    char addr_str[SIGAR_INET6_ADDRSTRLEN];

    if (len) {
        *buf = '\0';
    }

    list = mh_network_get_interfaces();
    for (plist = g_list_first(list); plist; plist = g_list_next(plist)) {
        struct mh_network_interface *mh_iface = plist->data;

        if ((g_str_equal(mh_network_interface_get_name(mh_iface), iface)) == TRUE) {
            sigar_net_address_to_string(NULL, &mh_iface->ifconfig.address, addr_str);
            strncpy(buf, addr_str, len - 1);
            buf[len - 1] = '\0';
            break;
        }
    }
    g_list_free_full(list, mh_network_interface_destroy);

    return buf;
}

const char *
mh_network_get_mac_address(const char *iface, char *buf, size_t len)
{
    GList *list = NULL;
    GList *plist;

    if (len) {
        *buf = '\0';
    }

    list = mh_network_get_interfaces();
    for (plist = g_list_first(list); plist; plist = g_list_next(plist)) {
        struct mh_network_interface *mh_iface = plist->data;

        if ((g_str_equal(mh_network_interface_get_name(mh_iface), iface)) == TRUE) {
            snprintf(buf, len, "%.2X:%.2X:%.2X:%.2X:%.2X:%.2X",
                           mh_iface->ifconfig.hwaddr.addr.mac[0],
                           mh_iface->ifconfig.hwaddr.addr.mac[1],
                           mh_iface->ifconfig.hwaddr.addr.mac[2],
                           mh_iface->ifconfig.hwaddr.addr.mac[3],
                           mh_iface->ifconfig.hwaddr.addr.mac[4],
                           mh_iface->ifconfig.hwaddr.addr.mac[5]);
            break;
        }
    }
    g_list_free_full(list, mh_network_interface_destroy);

    return buf;
}
