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
