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

static GList *
query_interface_table(void)
{
    int status;
    uint32_t lpc = 0;
    GList *interfaces = NULL;
    sigar_t* sigar;
    sigar_net_interface_list_t iflist;
    sigar_net_interface_config_t *ifconfig;

    sigar_open(&sigar);
    status = sigar_net_interface_list_get(sigar, &iflist);
    if(status == SIGAR_OK) {
	for(lpc = 0; lpc < iflist.number; lpc++) {
            ifconfig = g_new0(sigar_net_interface_config_t, 1);
	    status = sigar_net_interface_config_get(sigar, iflist.data[lpc],
						    ifconfig);
            if(status == SIGAR_OK)
                interfaces = g_list_prepend(interfaces, ifconfig);
        }
	sigar_net_interface_list_destroy(sigar, &iflist);
    }
    sigar_close(sigar);
    return interfaces;
}

GList *
mh_network_get_interfaces(void)
{
    GList *list = NULL;
    list = query_interface_table();
    return list;
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

void
mh_network_status(const char *iface, uint64_t *flags)
{
    GList *list = NULL;
    GList *plist;
    sigar_net_interface_config_t *ifconfig;

    list = mh_network_get_interfaces();
    for(plist = g_list_first(list); plist; plist = g_list_next(plist)) {
        ifconfig = (sigar_net_interface_config_t *)plist->data;
        if((g_str_equal(ifconfig->name, iface)) == TRUE) {
            *flags = ifconfig->flags;
        }
    }
}

const char *
mh_network_get_ip_address(const char *iface)
{
    GList *list = NULL;
    GList *plist;
    sigar_net_interface_config_t *ifconfig;
    char addr_str[SIGAR_INET6_ADDRSTRLEN];
    char *paddr_str;

    list = mh_network_get_interfaces();
    for(plist = g_list_first(list); plist; plist = g_list_next(plist)) {
        ifconfig = (sigar_net_interface_config_t *)plist->data;
        if((g_str_equal(ifconfig->name, iface)) == TRUE) {
            sigar_net_address_to_string(NULL, &ifconfig->address, addr_str);
            paddr_str = g_strdup(addr_str);
            return paddr_str;
        }
    }
    return NULL;
}

const char *
mh_network_get_mac_address(const char *iface)
{
    GList *list = NULL;
    GList *plist;
    sigar_net_interface_config_t *ifconfig;
    char *mac;

    list = mh_network_get_interfaces();
    for(plist = g_list_first(list); plist; plist = g_list_next(plist)) {
        ifconfig = (sigar_net_interface_config_t *)plist->data;
        if((g_str_equal(ifconfig->name, iface)) == TRUE) {
            mac = g_strdup_printf("%.2X:%.2X:%.2X:%.2X:%.2X:%.2X",
                                  ifconfig->hwaddr.addr.mac[0],
                                  ifconfig->hwaddr.addr.mac[1],
                                  ifconfig->hwaddr.addr.mac[2],
                                  ifconfig->hwaddr.addr.mac[3],
                                  ifconfig->hwaddr.addr.mac[4],
                                  ifconfig->hwaddr.addr.mac[5]);
            return mac;
        }
    }
    return NULL;
}
