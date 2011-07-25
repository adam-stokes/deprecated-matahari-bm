/*
 * mh_dbus_common.h
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

#ifndef MH_DBUS_COMMON_H
#define MH_DBUS_COMMON_H

#include <glib.h>
#include <dbus/dbus-glib.h>

/* GObject class definition */
#include "matahari/gobject_class.h"

/* Private struct in Matahari class */
#define MATAHARI_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
        MATAHARI_TYPE, MatahariPrivate))

// Matahari error codes
#define MATAHARI_ERROR matahari_error_quark ()
enum { MATAHARI_AUTHENTICATION_ERROR, MATAHARI_NOT_IMPLEMENTED };

GQuark
matahari_error_quark (void);

/**
 * Check the authorization for given 'action' using PolicyKit. Returns TRUE if
 * user is authorized, otherwise return FALSE and 'error' is set.
 */
gboolean
check_authorization(const gchar *action, GError** error,
                    DBusGMethodInvocation *context);

typedef struct {
    int prop;
    gchar *name, *nick, *desc;
    GParamFlags flags;
    char type;
} Property;

// This array is defined in auto-generated matahari-module-properties.h
extern Property properties[];

/**
 * Start DBus server with name 'bus_name' and object path 'object_path'
 */
int
run_dbus_server(char *bus_name, char *object_path);

/**
 * Check the authorization for getting the parameter 'name' using PolicyKit
 * action interface.name
 */
gboolean
matahari_get(Matahari* matahari, const char *interface, const char *name,
             DBusGMethodInvocation *context);

/**
 * Check the authorization for setting the parameter 'name' using PolicyKit
 * action interface.name
 */
gboolean
matahari_set(Matahari *matahari, const char *interface, const char *name,
             GValue *value, DBusGMethodInvocation *context);

/**
 * This method is used for getting value of DBus property.
 * It must be implemented in each module.
 * Set the value of property_id parameter to parameter value.
 */
void
matahari_get_property(GObject *object, guint property_id, GValue *value,
                      GParamSpec *pspec);

/**
 * This method is used for setting value of DBus property.
 * It must be implemented in each module.
 * New value of property_id is in parameter value.
 */
void
matahari_set_property(GObject *object, guint property_id, const GValue *value,
                      GParamSpec *pspec);

/**
 * This method is used to determine type of value in all dictionary parameters.
 * It must be implemented in each module.
 */
GType
matahari_dict_type(int prop);

/**
 * Dictionary type. Key is always string, value is set in constructor dict_new.
 */
typedef struct
{
    GValue *value;
    GValue *key;
    DBusGTypeSpecializedAppendContext appendctx;
} Dict;

/**
 * Constructor of dictionary type. Key is string, type of value is defined
 * by type of 'value'.
 * New value will be stored in 'value'. Use dict_free after setting all values.
 */
Dict *dict_new(GValue *value);

/**
 * Add 'value' to the dictionary 'dict'.
 */
void dict_add(Dict *dict, const gchar *key, GValue *value);

/**
 * Frees memory allocated by dict_new.
 */
void dict_free(Dict *dict);

#endif
