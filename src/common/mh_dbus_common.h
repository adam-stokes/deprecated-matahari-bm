#ifndef MH_DBUS_COMMON_H
#define MH_DBUS_COMMON_H

#include <glib.h>
#include <dbus/dbus-glib.h>

#define MATAHARI_ERROR matahari_error_quark ()
enum { MATAHARI_AUTHENTICATION_ERROR };

GQuark
matahari_error_quark (void);

gboolean
check_authorization(const gchar *action, GError** error, DBusGMethodInvocation *context);

#endif
