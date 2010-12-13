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

#include <stdlib.h>
#include <string.h>

#include <glib-object.h>
#include <glib/gi18n.h>
#include <dbus/dbus-glib-bindings.h>

/* GObject class definition */
#include "fmci/fmci-class.h"

/* DBus names */
#include "fmci/fmci-host-dbus.h"

/* Host methods */
#include "matahari/host.h"
#include "fmci/host.h"

/* Generated dbus stuff for host */
#include "matahari-host-dbus-glue.h"


/* Generate the GObject boilerplate */
G_DEFINE_TYPE(Fmci, fmci, G_TYPE_OBJECT)

/* Class init */
static void
fmci_class_init(FmciClass *fmci_class)
{
  dbus_g_object_type_install_info(FMCI_TYPE, &dbus_glib_fmci_object_info);
}

/* Instance init */
static void
fmci_init(Fmci *fmci)
{
}

int
main(int argc, char** argv)
{
  GMainLoop* loop = NULL;
  DBusGConnection *connection = NULL;
  GError *error = NULL;
  GObject *obj = NULL;
  DBusGProxy *driver_proxy = NULL;
  guint32 request_name_ret;

  g_type_init();
  loop = g_main_loop_new(NULL, FALSE);

  /* Obtain a connection to the system bus */
  connection = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);
  if (!connection)
    {
      g_printerr(_("Failed to open connection to bus: %s"), error->message);
      g_error_free(error);
      exit(1);
    }

  obj = g_object_new(FMCI_TYPE, NULL);
  dbus_g_connection_register_g_object(connection, FMCI_OBJECT_PATH, obj);

  driver_proxy = dbus_g_proxy_new_for_name(connection, DBUS_SERVICE_DBUS,
      DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS);

  if (!org_freedesktop_DBus_request_name(driver_proxy, FMCI_BUS_NAME, 0,
      &request_name_ret, &error))
    {
      g_printerr(_("Failed to get name: %s"), error->message);
      g_error_free(error);
      exit(1);
    }

  switch (request_name_ret)
    {
  case DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER:
    //g_print("OK, we are primary owner\n");
    break;
  case DBUS_REQUEST_NAME_REPLY_IN_QUEUE:
    g_printerr(
        _("Looks like another server of this type is already running: Reply in queue\n"));
    exit(1);
  case DBUS_REQUEST_NAME_REPLY_EXISTS:
    g_printerr(
        _("Looks like another server of this type is already running: Reply exists\n"));
    exit(1);
  case DBUS_REQUEST_NAME_REPLY_ALREADY_OWNER:
    g_printerr(_("We are already running\n"));
    exit(1);
  default:
    g_printerr(_("Unspecified error\n"));
    exit(1);
    }

  g_main_loop_run(loop);
  return 0;
}
