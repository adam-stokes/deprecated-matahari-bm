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
#include "mh_gobject_class.h"

/* Private struct in Matahari class */
#define MATAHARI_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MATAHARI_TYPE, MatahariPrivate))

#define MATAHARI_ERROR matahari_error_quark ()
enum { MATAHARI_AUTHENTICATION_ERROR, MATAHARI_NOT_IMPLEMENTED };

GQuark
matahari_error_quark (void);

gboolean
check_authorization(const gchar *action, GError** error, DBusGMethodInvocation *context);

enum Prop;

typedef struct {
    int prop;
    gchar *name, *nick, *desc;
    GParamFlags flags;
    char type;
} Property;

extern Property properties[];

gboolean
get_paramspec_from_property(Property prop, GParamSpec** spec);

int
run_dbus_server();

gboolean
matahari_get(Matahari* matahari, const char *interface, const char *name, DBusGMethodInvocation *context);

gboolean
matahari_set(Matahari *matahari, const char *interface, const char *name, GValue *value, DBusGMethodInvocation *context);

void
matahari_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec);

void
matahari_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);

GType
matahari_dict_type(int prop);

#endif
