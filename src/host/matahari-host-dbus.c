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
#include <stdlib.h>
#include <string.h>

#include <glib-object.h>
#include <glib/gi18n.h>
#include <dbus/dbus-glib-bindings.h>

/* GObject class definition */
#include "mh_gobject_class.h"

/* Host methods */
#include "matahari/host.h"

/* Generated properties list */
#include "matahari-host-dbus-properties.h"

/* DBus names */
#define HOST_BUS_NAME "org.matahariproject.Host"
#define HOST_OBJECT_PATH "/org/matahariproject/Host"
#define HOST_INTERFACE_NAME "org.matahariproject.Host"

/* Private struct in Matahari class */
#define MATAHARI_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MATAHARI_TYPE, MatahariPrivate))

struct _MatahariPrivate
{
  guint update_interval;
};

/* Dbus methods */
gboolean
Host_identify(Matahari* matahari, GError** error)
{
//  host_identify(5);
//  fprintf(stderr, "host_identify() is missing\n");
  return TRUE;
}

gboolean
Host_shutdown(Matahari* matahari, GError** error)
{
  host_shutdown();
  return TRUE;
}

gboolean
Host_reboot(Matahari* matahari, GError** error)
{
  host_reboot();
  return TRUE;
}

/* Generated dbus stuff for host
 * MUST be after declaration of user defined functions.
 */
#include "matahari-host-dbus-glue.h"


//TODO: Properties get/set
static void
matahari_set_property(GObject *object, guint property_id, const GValue *value,
    GParamSpec *pspec)
{
  Matahari *self = MATAHARI(object);
  switch (property_id)
    {
  case PROP_UPDATE_INTERVAL:
    self->priv->update_interval = g_value_get_uint (value);
    break;
  default:
    /* We don't have any other property... */
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
    }
}

static void
matahari_get_property(GObject *object, guint property_id, GValue *value,
    GParamSpec *pspec)
{
  Matahari *self = MATAHARI(object);
  sigar_loadavg_t load;
  sigar_proc_stat_t procs;

  switch (property_id)
    {
  case PROP_UUID:
    g_value_set_string (value, host_get_uuid());
    break;
  case PROP_HOSTNAME:
    g_value_set_string (value, host_get_hostname());
    break;
  case PROP_OS:
    g_value_set_string (value, host_get_operating_system());
    break;
  case PROP_ARCH:
    g_value_set_string (value, host_get_architecture());
    break;
  case PROP_WORDSIZE:
    g_value_set_uint (value, host_get_cpu_wordsize());
    break;
  case PROP_MEMORY:
    g_value_set_uint64 (value, host_get_memory());
    break;
  case PROP_SWAP:
    g_value_set_uint64 (value, host_get_swap());
    break;
  case PROP_CPU_COUNT:
    g_value_set_uint (value, host_get_cpu_count());
    break;
  case PROP_CPU_CORES:
    g_value_set_uint (value, host_get_cpu_number_of_cores());
    break;
  case PROP_CPU_MODEL:
    g_value_set_string (value, host_get_cpu_model());
    break;
  case PROP_CPU_FLAGS:
    g_value_set_string (value, host_get_cpu_flags());
    break;
  case PROP_UPDATE_INTERVAL:
    g_value_set_uint (value, self->priv->update_interval);
    break;
  case PROP_LAST_UPDATED:
    //TODO Logic here
    break;
  case PROP_SEQUENCE:
    //TODO Logic here
    break;
  case PROP_FREE_MEM:
    g_value_set_uint64 (value, host_get_mem_free());
    break;
  case PROP_FREE_SWAP:
    g_value_set_uint64 (value, host_get_swap_free());
    break;
  case PROP_LOAD:
    //TODO 1/5/15 minute load average - map
    break;
  case PROP_PROCESS_STATISTICS:
    //TODO proc stats - map
    break;
  default:
    /* We don't have any other property... */
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
    }
}

/* Generate the GObject boilerplate */
G_DEFINE_TYPE(Matahari, matahari, G_TYPE_OBJECT)

/* Class init */
static void
matahari_class_init(MatahariClass *matahari_class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(matahari_class);
  GParamSpec *pspec;

  g_type_class_add_private(matahari_class, sizeof (MatahariPrivate));

  gobject_class->set_property = matahari_set_property;
  gobject_class->get_property = matahari_get_property;

  //TODO: Proper properties initialization
  int i;
  for (i = 0; properties_Host[i].name != NULL; i++)
  {
    g_print("Writing property: %s\n", properties_Host[i].name);
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
            pspec = g_param_spec_int(properties_Host[i].name,
                                      properties_Host[i].nick,
                                      properties_Host[i].desc,
                                      G_MININT, G_MAXINT, 0,
                                      properties_Host[i].flags);
            break;
        case 'x':
            pspec = g_param_spec_int64(properties_Host[i].name,
                                       properties_Host[i].nick,
                                       properties_Host[i].desc,
                                       G_MININT64, G_MAXINT64, 0,
                                       properties_Host[i].flags);
 
            break;
        case 'y':
        case 'q':
        case 'u':
            pspec = g_param_spec_uint(properties_Host[i].name,
                                         properties_Host[i].nick,
                                         properties_Host[i].desc,
                                         0, G_MAXUINT, 0,
                                         properties_Host[i].flags);
            break;
        case 't':
            pspec = g_param_spec_uint64(properties_Host[i].name,
                                         properties_Host[i].nick,
                                         properties_Host[i].desc,
                                         0, G_MAXUINT64, 0,
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
            g_printerr("Unknown type: %c\n", properties_Host[i].type);
            pspec = NULL;
    }
    if (pspec)
        g_object_class_install_property(gobject_class, properties_Host[i].prop, pspec);
  }

  dbus_g_object_type_install_info(MATAHARI_TYPE, &dbus_glib_matahari_object_info);
}

/* Instance init */
static void
matahari_init(Matahari *matahari)
{
  MatahariPrivate *priv;
  matahari->priv = priv = MATAHARI_GET_PRIVATE(matahari);
  //XXX hardcoded number
  priv->update_interval = 5;
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

  obj = g_object_new(MATAHARI_TYPE, NULL);
  dbus_g_connection_register_g_object(connection, HOST_OBJECT_PATH, obj);

  driver_proxy = dbus_g_proxy_new_for_name(connection, DBUS_SERVICE_DBUS,
      DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS);

  if (!org_freedesktop_DBus_request_name(driver_proxy, HOST_BUS_NAME, 0,
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
