/*
 * matahari-host-dbus.c
 *
 * Copyright (C) 2010 Red Hat, Inc.
 * Written by Roman Rakus <rrakus@redhat.com>
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

/* Host methods */
#include "matahari/host.h"

/* Generated properties list */
#include "host-dbus-properties.h"

/* DBus names */
#define HOST_BUS_NAME "org.matahariproject.Host"
#define HOST_OBJECT_PATH "/org/matahariproject/Host"
#define HOST_INTERFACE_NAME "org.matahariproject.Host"
#define DBUS_PROPERTY_INTERAFACE_NAME "org.freedesktop.DBus.Properties"

struct Private
{
    guint update_interval;
};

struct Private priv;

/* Dbus methods */
gboolean
Host_identify(Matahari* matahari, DBusGMethodInvocation *context)
{
    GError* error = NULL;
    if (!check_authorization(HOST_BUS_NAME ".identify", &error, context)) {
        dbus_g_method_return_error(context, error);
        return FALSE;
    }
    mh_host_identify();
    dbus_g_method_return(context, TRUE);
    return TRUE;
}

gboolean
Host_shutdown(Matahari* matahari, DBusGMethodInvocation *context)
{
    GError* error = NULL;
    if (!check_authorization(HOST_BUS_NAME ".shutdown", &error, context)) {
        dbus_g_method_return_error(context, error);
        return FALSE;
    }
    mh_host_shutdown();
    dbus_g_method_return(context, TRUE);
    return TRUE;
}

gboolean
Host_reboot(Matahari* matahari, DBusGMethodInvocation *context)
{
    GError* error = NULL;
    if (!check_authorization(HOST_BUS_NAME ".reboot", &error, context)) {
        dbus_g_method_return_error(context, error);
        return FALSE;
    }
    mh_host_reboot();
    dbus_g_method_return(context, TRUE);
    return TRUE;
}

gboolean
Host_get_uuid(Matahari* matahari, const char *lifetime, DBusGMethodInvocation *context)
{
    GError* error = NULL;
    const char *uuid = NULL;
    if (!check_authorization(HOST_BUS_NAME ".get_uuid", &error, context)) {
        dbus_g_method_return_error(context, error);
        return FALSE;
    }
    uuid = mh_host_get_uuid(lifetime);
    dbus_g_method_return(context, uuid);
    return TRUE;
}

gboolean
Host_set_uuid(Matahari* matahari, const char *lifetime, const char *uuid, DBusGMethodInvocation *context)
{
    int rc = 0;
    GError* error = NULL;
    if (!check_authorization(HOST_BUS_NAME ".set_uuid", &error, context)) {
        dbus_g_method_return_error(context, error);
        return FALSE;
    }
    rc = mh_host_set_uuid(lifetime, uuid);
    dbus_g_method_return(context, rc);
    return TRUE;
}

/* Generated dbus stuff for host
 * MUST be after declaration of user defined functions.
 */
#include "host-dbus-glue.h"

void
matahari_set_property(GObject *object, guint property_id, const GValue *value,
                      GParamSpec *pspec)
{
    switch (property_id) {
    case PROP_HOST_UPDATE_INTERVAL:
        priv.update_interval = g_value_get_uint (value);
        break;
    default:
        /* We don't have any other property... */
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

void
matahari_get_property(GObject *object, guint property_id, GValue *value,
                      GParamSpec *pspec)
{
    sigar_proc_stat_t procs;
    sigar_loadavg_t avg;
    Dict *dict;
    GValue value_value = {0, };

    switch (property_id) {
    case PROP_HOST_UUID:
        g_value_set_string (value, mh_host_get_uuid(NULL));
        break;
    case PROP_HOST_HOSTNAME:
        g_value_set_string (value, mh_host_get_hostname());
        break;
    case PROP_HOST_OS:
        g_value_set_string (value, mh_host_get_operating_system());
        break;
    case PROP_HOST_ARCH:
        g_value_set_string (value, mh_host_get_architecture());
        break;
    case PROP_HOST_WORDSIZE:
        g_value_set_uint (value, mh_host_get_cpu_wordsize());
        break;
    case PROP_HOST_MEMORY:
        g_value_set_uint64 (value, mh_host_get_memory());
        break;
    case PROP_HOST_SWAP:
        g_value_set_uint64 (value, mh_host_get_swap());
        break;
    case PROP_HOST_CPU_COUNT:
        g_value_set_uint (value, mh_host_get_cpu_count());
        break;
    case PROP_HOST_CPU_CORES:
        g_value_set_uint (value, mh_host_get_cpu_number_of_cores());
        break;
    case PROP_HOST_CPU_MODEL:
        g_value_set_string (value, mh_host_get_cpu_model());
        break;
    case PROP_HOST_CPU_FLAGS:
        g_value_set_string (value, mh_host_get_cpu_flags());
        break;
    case PROP_HOST_UPDATE_INTERVAL:
        g_value_set_uint (value, priv.update_interval);
        break;
    case PROP_HOST_LAST_UPDATED:
        // Not used in DBus module
        break;
    case PROP_HOST_SEQUENCE:
        // Not used in DBus module
        break;
    case PROP_HOST_FREE_MEM:
        g_value_set_uint64 (value, mh_host_get_mem_free());
        break;
    case PROP_HOST_FREE_SWAP:
        g_value_set_uint64 (value, mh_host_get_swap_free());
        break;
    case PROP_HOST_LOAD:
        // 1/5/15 minute load average - map
        mh_host_get_load_averages(&avg);

        dict = dict_new(value);
        g_value_init (&value_value, G_TYPE_DOUBLE);

        g_value_set_double(&value_value, avg.loadavg[0]);
        dict_add(dict, "1", &value_value);

        g_value_set_double(&value_value, avg.loadavg[1]);
        dict_add(dict, "5", &value_value);

        g_value_set_double(&value_value, avg.loadavg[2]);
        dict_add(dict, "15", &value_value);
        dict_free(dict);
        break;
    case PROP_HOST_PROCESS_STATISTICS:
        // Process statistics is type map string -> int
        mh_host_get_processes(&procs);

        dict = dict_new(value);
        g_value_init (&value_value, G_TYPE_INT);

        g_value_set_int(&value_value, procs.total);
        dict_add(dict, "total", &value_value);

        g_value_set_int(&value_value, procs.idle);
        dict_add(dict, "idle", &value_value);

        g_value_set_int(&value_value, procs.zombie);
        dict_add(dict, "zombie", &value_value);

        g_value_set_int(&value_value, procs.running);
        dict_add(dict, "running", &value_value);

        g_value_set_int(&value_value, procs.stopped);
        dict_add(dict, "stopped", &value_value);

        g_value_set_int(&value_value, procs.sleeping);
        dict_add(dict, "sleeping", &value_value);
        dict_free(dict);
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
    switch (prop) {
    case PROP_HOST_LOAD:
        return G_TYPE_DOUBLE;
        break;
    case PROP_HOST_PROCESS_STATISTICS:
        return G_TYPE_INT;
        break;
    default:
        g_printerr("Type of property %s is map of unknown types\n",
                   properties[prop].name);
        return G_TYPE_VALUE;
    }
}

int
main(int argc, char** argv)
{
    g_type_init();
    priv.update_interval = 5;
    return run_dbus_server(HOST_BUS_NAME, HOST_OBJECT_PATH);
}
