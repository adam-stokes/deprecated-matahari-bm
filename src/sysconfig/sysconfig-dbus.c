/*
 * sysconfig-dbus.c
 *
 * Copyright (C) 2011 Red Hat, Inc.
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

#include "config.h"
#include "matahari/sysconfig.h"
#include "matahari/utilities.h"
#include "matahari/dbus_common.h"

/* Generated properties list */
#include "sysconfig-dbus-properties.h"

/* DBus names */
#define SYSCONFIG_BUS_NAME "org.matahariproject.Sysconfig"
#define SYSCONFIG_OBJECT_PATH "/org/matahariproject/Sysconfig"
#define SYSCONFIG_INTERFACE_NAME "org.matahariproject.Sysconfig"
#define DBUS_PROPERTY_INTERAFACE_NAME "org.freedesktop.DBus.Properties"

void result_cb(void *data, int result)
{
}

gboolean
Sysconfig_run_uri(Matahari* matahari, const char *uri, uint flags,
                  const char *scheme, const char *key,
                  DBusGMethodInvocation *context)
{
    GError* error = NULL;
    char *status;
    enum mh_result res;

    if (!check_authorization(SYSCONFIG_BUS_NAME ".run_uri", &error, context)) {
        dbus_g_method_return_error(context, error);
        g_error_free(error);
        return FALSE;
    }

    res = mh_sysconfig_run_uri(uri, flags, scheme, key, result_cb, NULL);

    if (res == MH_RES_SUCCESS) {
        status = mh_sysconfig_is_configured(key);
        dbus_g_method_return(context, status);
        free(status);
    } else {
        error = g_error_new(MATAHARI_ERROR, res, mh_result_to_str(res));
        dbus_g_method_return_error(context, error);
        g_error_free(error);
        return FALSE;
    }
    return TRUE;
}

gboolean
Sysconfig_run_string(Matahari* matahari, const char *text, uint flags,
                     const char *scheme, const char *key,
                     DBusGMethodInvocation *context)
{
    GError* error = NULL;
    char *status;
    enum mh_result res;

    if (!check_authorization(SYSCONFIG_BUS_NAME ".run_string", &error,
            context)) {
        dbus_g_method_return_error(context, error);
        g_error_free(error);
        return FALSE;
    }

    res = mh_sysconfig_run_string(text, flags, scheme, key, result_cb, NULL);

    if (res == MH_RES_SUCCESS) {
        status = mh_sysconfig_is_configured(key);
        dbus_g_method_return(context, status);
        free(status);
    } else {
        error = g_error_new(MATAHARI_ERROR, res, mh_result_to_str(res));
        dbus_g_method_return_error(context, error);
        g_error_free(error);
        return FALSE;
    }
    return TRUE;
}

gboolean
Sysconfig_query(Matahari* matahari, const char *text, uint flags,
                const char *scheme, DBusGMethodInvocation *context)
{
    GError* error = NULL;
    char *status;
    if (!check_authorization(SYSCONFIG_BUS_NAME ".query", &error, context)) {
        dbus_g_method_return_error(context, error);
        g_error_free(error);
        return FALSE;
    }

    status = mh_sysconfig_query(text, flags, scheme);

    dbus_g_method_return(context, status);
    free(status);
    return TRUE;
}

gboolean
Sysconfig_is_configured(Matahari* matahari, const char *key,
                        DBusGMethodInvocation *context)
{
    GError* error = NULL;
    char *status;
    if (!check_authorization(SYSCONFIG_BUS_NAME ".is_configured", &error,
            context)) {
        dbus_g_method_return_error(context, error);
        g_error_free(error);
        return FALSE;
    }

    status = mh_sysconfig_is_configured(key);

    dbus_g_method_return(context, status);
    free(status);
    return TRUE;
}

/* Generated dbus stuff for sysconfig
 * MUST be after declaration of user defined functions.
 */
#include "sysconfig-dbus-glue.h"

void
matahari_set_property(GObject *object, guint property_id, const GValue *value,
                      GParamSpec *pspec)
{
    /* We don't have any other property... */
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

void
matahari_get_property(GObject *object, guint property_id, GValue *value,
                      GParamSpec *pspec)
{
    switch (property_id) {
    case PROP_SYSCONFIG_UUID:
        g_value_set_string (value, mh_uuid());
        break;
    case PROP_SYSCONFIG_HOSTNAME:
        g_value_set_string (value, mh_hostname());
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
    return run_dbus_server(SYSCONFIG_BUS_NAME, SYSCONFIG_OBJECT_PATH);
}
