/*
 * mh_dbus_common.c
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

#include "matahari/dbus_common.h"
#include "matahari/logging.h"
#include <glib/gi18n.h>

#include <polkit/polkit.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <dbus/dbus-glib-bindings.h>

/* Generate the GObject boilerplate */
G_DEFINE_TYPE(Matahari, matahari, G_TYPE_OBJECT)

const DBusGObjectInfo dbus_glib_matahari_object_info;

GQuark
matahari_error_quark (void)
{
    return g_quark_from_static_string ("matahari-error-quark");
}


gboolean
check_authorization(const gchar *action, GError** error,
                    DBusGMethodInvocation *context)
{
    GError *err;
    PolkitAuthorizationResult *result;
    PolkitSubject *subject;
    PolkitAuthority *authority;
    gboolean res;

    if (context == NULL) {
        g_printerr("Context is not set!\n");
        return FALSE;
    }
    err = NULL;
    subject = polkit_system_bus_name_new(dbus_g_method_get_sender(context));
#ifndef HAVE_PK_GET_SYNC
    authority = polkit_authority_get();
    if(!authority) {
        g_printerr("Error in obtaining authority\n");
        return FALSE;
    }
#else
    authority = polkit_authority_get_sync(NULL, &err);
    if (err != NULL) {
        g_printerr("Error in obtaining authority: %s\n", err->message);
        g_set_error(error, MATAHARI_ERROR, MATAHARI_AUTHENTICATION_ERROR,
                    err->message);
        g_error_free(err);
        return FALSE;
    }
#endif
    result = polkit_authority_check_authorization_sync(authority, subject,
            action, NULL,
            POLKIT_CHECK_AUTHORIZATION_FLAGS_ALLOW_USER_INTERACTION, NULL,
            &err);
    if (err != NULL) {
        g_printerr("Error in checking authorization: %s\n", err->message);
        g_set_error(error, MATAHARI_ERROR, MATAHARI_AUTHENTICATION_ERROR,
                    err->message);
        g_error_free(err);
        return FALSE;
    }
    res = polkit_authorization_result_get_is_authorized(result);
    g_object_unref(subject);
    g_object_unref(result);
    g_object_unref(authority);
    if (!res) {
        g_set_error(error, MATAHARI_ERROR, MATAHARI_AUTHENTICATION_ERROR,
                    "You are not authorized for specified action");
        g_printerr("Caller is not authorized for action %s\n", action);
    }
    return res;
}

gboolean
get_paramspec_from_property(Property prop, GParamSpec** pspec)
{
    GType value_type;
    switch (prop.type) {
    case 's':
        *pspec = g_param_spec_string(prop.name, prop.nick, prop.desc,
                                     NULL, prop.flags);
        break;
    case 'b':
        *pspec = g_param_spec_boolean(prop.name, prop.nick, prop.desc,
                                      FALSE, prop.flags);
        break;
    case 'n':
    case 'i':
        *pspec = g_param_spec_int(prop.name, prop.nick, prop.desc,
                                  G_MININT, G_MAXINT, 0, prop.flags);
        break;
    case 'x':
        *pspec = g_param_spec_int64(prop.name, prop.nick, prop.desc,
                                    G_MININT64, G_MAXINT64, 0, prop.flags);

        break;
    case 'y':
    case 'q':
    case 'u':
        *pspec = g_param_spec_uint(prop.name, prop.nick, prop.desc,
                                   0, G_MAXUINT, 0, prop.flags);
        break;
    case 't':
        *pspec = g_param_spec_uint64(prop.name, prop.nick, prop.desc,
                                     0, G_MAXUINT64, 0, prop.flags);
        break;
    case 'd':
        *pspec = g_param_spec_double(prop.name, prop.nick, prop.desc,
                                     -G_MAXDOUBLE, G_MAXDOUBLE, 0,
                                     prop.flags);
        break;
    case 'e':
        // Type is map - key is string, type of value must be added manually!
        value_type = matahari_dict_type(prop.prop);
        *pspec = g_param_spec_boxed(prop.name, prop.nick, prop.desc,
                                    dbus_g_type_get_map("GHashTable",
                                                        G_TYPE_STRING,
                                                        value_type),
                                    prop.flags);
        break;
    default:
        return FALSE;
    }
    return TRUE;

}

int
run_dbus_server(char *bus_name, char *object_path)
{
    GMainLoop* loop = NULL;
    DBusGConnection *connection = NULL;
    GError *error = NULL;
    GObject *obj = NULL;
    DBusGProxy *driver_proxy = NULL;
    guint32 request_name_ret;

    mh_log_init(bus_name, LOG_DEBUG, FALSE);

    loop = g_main_loop_new(NULL, FALSE);

    // Obtain a connection to the system bus
    connection = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);
    if (!connection) {
        g_printerr(_("Failed to open connection to bus: %s\n"), error->message);
        g_error_free(error);
        return 1;
    }

    obj = g_object_new(MATAHARI_TYPE, NULL);
    dbus_g_connection_register_g_object(connection, object_path, obj);

    driver_proxy = dbus_g_proxy_new_for_name(connection, DBUS_SERVICE_DBUS,
                                             DBUS_PATH_DBUS,
                                             DBUS_INTERFACE_DBUS);

    if (!org_freedesktop_DBus_request_name(driver_proxy, bus_name, 0,
                                           &request_name_ret, &error)) {
        g_printerr(_("Failed to get name: %s\n"), error->message);
        g_error_free(error);
        return 1;
    }

    switch (request_name_ret) {
    case DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER:
        break;
    case DBUS_REQUEST_NAME_REPLY_IN_QUEUE:
        g_printerr(_("Looks like another server of this type is already "
                     "running: Reply in queue\n"));
        return 1;
    case DBUS_REQUEST_NAME_REPLY_EXISTS:
        g_printerr(_("Looks like another server of this type is already "
                     "running: Reply exists\n"));
        return 1;
    case DBUS_REQUEST_NAME_REPLY_ALREADY_OWNER:
        g_printerr(_("We are already running\n"));
        return 1;
    default:
        g_printerr(_("Unspecified error\n"));
        return 1;
    }

    g_main_loop_run(loop);
    g_main_loop_unref(loop);
    g_object_unref(obj);
    g_object_unref(driver_proxy);
    return 0;
}

gboolean
matahari_get(Matahari* matahari, const char *interface, const char *name,
             DBusGMethodInvocation *context)
{
    GError* error = NULL;
    GParamSpec *spec;
    GValue value = {0, };

    char *action = malloc((strlen(interface) + strlen(name) + 2) * sizeof(char));
    sprintf(action, "%s.%s", interface, name);
    if (!check_authorization(action, &error, context)) {
        dbus_g_method_return_error(context, error);
        free(action);
        return FALSE;
    }
    free(action);

    spec = g_object_class_find_property(G_OBJECT_GET_CLASS(matahari), name);
    g_value_init(&value, spec->value_type);
    g_object_get_property(G_OBJECT(matahari), name, &value);
    dbus_g_method_return(context, &value);
    return TRUE;
}

gboolean
matahari_set(Matahari *matahari, const char *interface, const char *name,
             GValue *value, DBusGMethodInvocation *context)
{
    GError* error = NULL;
    char *action = malloc((strlen(interface) + strlen(name) + 2) * sizeof(char));
    sprintf(action, "%s.%s", interface, name);
    if (!check_authorization(action, &error, context)) {
        dbus_g_method_return_error(context, error);
        free(action);
        return FALSE;
    }
    free(action);

    g_object_set_property(G_OBJECT(matahari), name, value);
    dbus_g_method_return(context);
    return TRUE;
}

/* Class init */
static void
matahari_class_init(MatahariClass *matahari_class)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(matahari_class);
    GParamSpec *pspec = NULL;

    gobject_class->set_property = matahari_set_property;
    gobject_class->get_property = matahari_get_property;

    int i;
    for (i = 0; properties[i].name != NULL; i++) {
        if (!get_paramspec_from_property(properties[i], &pspec)) {
            g_printerr("Unknown type: %c\n", properties[i].type);
            pspec = NULL;
        }
        if (pspec)
            g_object_class_install_property(gobject_class, properties[i].prop,
                                            pspec);
    }

    dbus_g_object_type_install_info(MATAHARI_TYPE,
                                    &dbus_glib_matahari_object_info);
}

/* Instance init */
static void
matahari_init(Matahari *matahari)
{
    matahari->priv = MATAHARI_GET_PRIVATE(matahari);
}

Dict *
dict_new(GValue *value)
{
    gpointer ret;
    Dict *dict = malloc(sizeof(Dict));
    dict->key = calloc(sizeof(GValue), 1);
    dict->value = value;
    g_value_init(dict->key, G_TYPE_STRING);
    ret = dbus_g_type_specialized_construct (G_VALUE_TYPE (value));
    g_value_set_boxed_take_ownership (value, ret);

    dbus_g_type_specialized_init_append (value, &(dict->appendctx));

    return dict;
}

void
dict_add(Dict *dict, const gchar *key, GValue *value)
{
    g_value_set_static_string(dict->key, key);
    dbus_g_type_specialized_map_append (&(dict->appendctx), dict->key, value);
}

void
dict_free(Dict *dict)
{
    free(dict->key);
    free(dict);
}
