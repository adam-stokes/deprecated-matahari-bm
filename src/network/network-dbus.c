/*
 * matahari-network-dbus.c
 *
 * Copyright (C) 2010 Red Hat, Inc.
 * Written by Radek Novacek <rnovacek@redhat.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include <stdio.h>
#include <string.h>

#include "matahari/dbus_common.h"

/* Network methods */
#include "matahari/network.h"
#include "matahari/utilities.h"

/* Generated properties list */
#include "network-dbus-properties.h"

/* DBus names */
#define NETWORK_BUS_NAME "org.matahariproject.Network"
#define NETWORK_OBJECT_PATH "/org/matahariproject/Network"
#define NETWORK_INTERFACE_NAME "org.matahariproject.Network"


enum status { INACTIVE = 0, RUNNING };

/* Get status of the interface */
static enum status
interface_status(const char *iface)
{
    uint64_t flags = 0;

    mh_network_status(iface, &flags);

    if (flags & MH_NETWORK_IF_UP) {
        return RUNNING;
    }

    return INACTIVE; /* Inactive */
}

/* Dbus methods */

gboolean
Network_list(Matahari *matahari, DBusGMethodInvocation *context)
{
    GError* error = NULL;
    GList *interface_list;
    GList *plist;
    char **list;
    int i = 0;

    if (!check_authorization(NETWORK_BUS_NAME ".list", &error, context)) {
        dbus_g_method_return_error(context, error);
        return FALSE;
    }

    // Get the list of interfaces
    interface_list = mh_network_get_interfaces();

    // Alloc array for list of interface names
    list = g_new(char *, g_list_length(interface_list) + 1);

    // Convert list of interfaces to the array of names
    for (plist = g_list_first(interface_list);
         plist;
         plist = g_list_next(plist)) {
        struct mh_network_interface *iface = plist->data;
        list[i++] = strdup(mh_network_interface_get_name(iface));
    }
    list[i] = NULL; // Sentinel

    dbus_g_method_return(context, list);
    g_strfreev(list);
    g_list_free_full(interface_list, mh_network_interface_destroy);
    return TRUE;
}

gboolean
Network_start(Matahari *matahari, const char *iface,
              DBusGMethodInvocation *context)
{
    GError* error = NULL;
    int status;

    if (!check_authorization(NETWORK_BUS_NAME ".start", &error, context)) {
        dbus_g_method_return_error(context, error);
        return FALSE;
    }

    status = interface_status(iface);
    if (status != RUNNING) {
        mh_network_start(iface);
        status = interface_status(iface);
    }
    dbus_g_method_return(context, status);
    return TRUE;
}

gboolean
Network_stop(Matahari *matahari, const char *iface,
             DBusGMethodInvocation *context)
{
    GError* error = NULL;
    int status;

    if (!check_authorization(NETWORK_BUS_NAME ".stop", &error, context)) {
        dbus_g_method_return_error(context, error);
        return FALSE;
    }

    status = interface_status(iface);
    if (status != INACTIVE) {
        mh_network_stop(iface);
        status = interface_status(iface);
    }
    dbus_g_method_return(context, status);
    return TRUE;
}

gboolean
Network_status(Matahari *matahari, const char *iface,
               DBusGMethodInvocation *context)
{
    GError* error = NULL;
    if (!check_authorization(NETWORK_BUS_NAME ".status", &error, context)) {
        dbus_g_method_return_error(context, error);
        return FALSE;
    }

    dbus_g_method_return(context, interface_status(iface));
    return TRUE;
}

gboolean
Network_get_ip_address(Matahari *matahari, const char *iface,
                       DBusGMethodInvocation *context)
{
    char buf[64];
    GError* error = NULL;

    if (!check_authorization(NETWORK_BUS_NAME ".get_ip_address", &error,
                             context)) {
        dbus_g_method_return_error(context, error);
        return FALSE;
    }

    dbus_g_method_return(context,
            g_strdup(mh_network_get_ip_address(iface, buf, sizeof(buf))));

    return TRUE;
}

gboolean
Network_get_mac_address(Matahari *matahari, const char *iface,
                        DBusGMethodInvocation *context)
{
    GError* error = NULL;
    char buf[32];

    if (!check_authorization(NETWORK_BUS_NAME ".get_mac_address", &error,
                             context)) {
        dbus_g_method_return_error(context, error);
        return FALSE;
    }

    dbus_g_method_return(context,
            g_strdup(mh_network_get_mac_address(iface, buf, sizeof(buf))));

    return TRUE;
}

/* Generated dbus stuff for network
 * MUST be after declaration of user defined functions.
 */
#include "network-dbus-glue.h"

void
matahari_set_property(GObject *object, guint property_id, const GValue *value,
                      GParamSpec *pspec)
{
    // We don't have writable other property...
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

void
matahari_get_property(GObject *object, guint property_id, GValue *value,
                      GParamSpec *pspec)
{
    switch (property_id) {
    case PROP_NETWORK_HOSTNAME:
        g_value_set_string (value, mh_hostname());
        break;
    case PROP_NETWORK_UUID:
        g_value_set_string (value, mh_uuid());
        break;
    default:
        /* We don't have any other property... */
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

GType
matahari_dict_type(int prop)
{
    g_printerr("Type of property %s is map of unknown types\n",
               properties[prop].name);
    return G_TYPE_VALUE;
}

int
main(int argc, char** argv)
{
    g_type_init();
    return run_dbus_server(NETWORK_BUS_NAME, NETWORK_OBJECT_PATH);
}
