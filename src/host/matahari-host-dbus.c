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

/* Generated properties list */
#include "matahari-host-dbus-properties.h"


//TODO: Properties get/set
static void
fmci_set_property(GObject *object, guint property_id, const GValue *value,
    GParamSpec *pspec)
{
  Fmci *self = FMCI(object);
  switch (property_id)
    {
  case PROP_UUID:
  case PROP_HOSTNAME:
  case PROP_IS_VIRTUAL:
  case PROP_OPERATING_SYSTEM:
  case PROP_MEMORY:
  case PROP_SWAP:
  case PROP_ARCH:
  case PROP_PLATFORM:
  case PROP_PROCESSORS:
  case PROP_CORES:
  case PROP_MODEL:
  case PROP_LAST_UPDATED_SEQ:
  case PROP_LAST_UPDATED:
  case PROP_LOAD_AVERAGE_1:
  case PROP_LOAD_AVERAGE_5:
  case PROP_LOAD_AVERAGE_15:
  case PROP_MEM_FREE:
  case PROP_SWAP_FREE:
  case PROP_PROC_TOTAL:
  case PROP_PROC_RUNNING:
  default:
    /* We don't have any other property... */
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
    }
}

static void
fmci_get_property(GObject *object, guint property_id, GValue *value,
    GParamSpec *pspec)
{
  Fmci *self = FMCI(object);
  switch (property_id)
    {
  case PROP_UUID:
    g_value_set_string (value, host_get_uuid());
    break;
  case PROP_HOSTNAME:
    g_value_set_string (value, host_get_hostname());
    break;
  case PROP_IS_VIRTUAL:
    //TODO
    break;
  case PROP_OPERATING_SYSTEM:
    g_value_set_string (value, host_get_operating_system());
    break;
  case PROP_MEMORY:
  case PROP_SWAP:
  case PROP_ARCH:
  case PROP_PLATFORM:
  case PROP_PROCESSORS:
  case PROP_CORES:
  case PROP_MODEL:
  case PROP_LAST_UPDATED_SEQ:
  case PROP_LAST_UPDATED:
  case PROP_LOAD_AVERAGE_1:
  case PROP_LOAD_AVERAGE_5:
  case PROP_LOAD_AVERAGE_15:
  case PROP_MEM_FREE:
  case PROP_SWAP_FREE:
  case PROP_PROC_TOTAL:
  case PROP_PROC_RUNNING:
  default:
    /* We don't have any other property... */
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
    }
}

/* Generate the GObject boilerplate */
G_DEFINE_TYPE(Fmci, fmci, G_TYPE_OBJECT)

/* Class init */
static void
fmci_class_init(FmciClass *fmci_class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(fmci_class);
  GParamSpec *pspec;

  gobject_class->set_property = fmci_set_property;
  gobject_class->get_property = fmci_get_property;

  //TODO: Proper properties initialization
  int i;
  for (i = 0; properties_Host[i].name != NULL; i++)
  {
    printf("Writing property: %s\n", properties_Host[i].name);
    switch (properties_Host[i].type)
    {
        case 's':
            pspec = g_param_spec_string(properties_Host[i].name,
                                        properties_Host[i].nick,
                                        properties_Host[i].desc,
                                        NULL,
                                        properties_Host[i].flags);
            break;
        case 'b':
            pspec = g_param_spec_boolean(properties_Host[i].name,
                                         properties_Host[i].nick,
                                         properties_Host[i].desc,
                                         FALSE,
                                         properties_Host[i].flags);
            break;
        case 'n':
        case 'i':
        case 'x':
            pspec = g_param_spec_int(properties_Host[i].name,
                                      properties_Host[i].nick,
                                      properties_Host[i].desc,
                                      G_MININT, G_MAXINT, 0,
                                      properties_Host[i].flags);
            break;
        case 'y':
        case 'q':
        case 'u':
        case 't':
            pspec = g_param_spec_uint(properties_Host[i].name,
                                         properties_Host[i].nick,
                                         properties_Host[i].desc,
                                         0, G_MAXUINT, 0,
                                         properties_Host[i].flags);
            break;
        case 'd':
            pspec = g_param_spec_double(properties_Host[i].name,
                                         properties_Host[i].nick,
                                         properties_Host[i].desc,
                                         -G_MAXDOUBLE, G_MAXDOUBLE, 0,
                                         properties_Host[i].flags);
            break;
        default:
            fprintf(stderr, "Unknown type: %c\n", properties_Host[i].type);
            pspec = NULL;
    }
    if (pspec)
        g_object_class_install_property(gobject_class, properties_Host[i].prop, pspec);
  }

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
