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
#include <dbus/dbus-glib-bindings.h>

#include "mh_dbus_common.h"

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
#define DBUS_PROPERTY_INTERAFACE_NAME "org.freedesktop.DBus.Properties"

/* Private struct in Matahari class */
#define MATAHARI_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MATAHARI_TYPE, MatahariPrivate))

struct _MatahariPrivate
{
  guint update_interval;
};


/* Dbus methods */
gboolean
Host_identify(Matahari* matahari, DBusGMethodInvocation *context)
{
  GError* error = NULL;
  if (!check_authorization(HOST_BUS_NAME ".identify", &error, context))
  {
    dbus_g_method_return_error(context, error);
    return FALSE;
  }
//  host_identify(5); XXX, not implemented
  printf("Host_identify\n");
  dbus_g_method_return(context, TRUE);
  return TRUE;
}

gboolean
Host_shutdown(Matahari* matahari, DBusGMethodInvocation *context)
{
  GError* error = NULL;
  if (!check_authorization(HOST_BUS_NAME ".shutdown", &error, context))
  {
    dbus_g_method_return_error(context, error);
    return FALSE;
  }
  host_shutdown();
  dbus_g_method_return(context, TRUE);
  return TRUE;
}

gboolean
Host_reboot(Matahari* matahari, DBusGMethodInvocation *context)
{
  GError* error = NULL;
  if (!check_authorization(HOST_BUS_NAME ".reboot", &error, context))
  {
    dbus_g_method_return_error(context, error);
    return FALSE;
  }
  host_reboot();
  dbus_g_method_return(context, TRUE);
  return TRUE;
}

gboolean
matahari_get(Matahari* matahari, const char *interface, const char *name, DBusGMethodInvocation *context)
{
  GError* error = NULL;
  char *action = malloc((strlen(interface) + strlen(name) + 2) * sizeof(char));
  sprintf(action, "%s.%s", interface, name);
  if (!check_authorization(action, &error, context))
  {
    dbus_g_method_return_error(context, error);
    free(action);
    return FALSE;
  }
  free(action);

  GParamSpec *spec = g_object_class_find_property(G_OBJECT_GET_CLASS(matahari), name);
  GValue value = {0, };
  g_value_init(&value, spec->value_type);
  g_object_get_property(G_OBJECT(matahari), name, &value);
  dbus_g_method_return(context, &value);
  return TRUE;
}

gboolean
matahari_set(Matahari *matahari, const char *interface, const char *name, GValue *value, DBusGMethodInvocation *context)
{
  GError* error = NULL;
  char *action = malloc((strlen(interface) + strlen(name) + 2) * sizeof(char));
  sprintf(action, "%s.%s", interface, name);
  if (!check_authorization(action, &error, context))
  {
    dbus_g_method_return_error(context, error);
    free(action);
    return FALSE;
  }
  free(action);

  g_object_set_property(G_OBJECT(matahari), name, value);
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
  sigar_proc_stat_t procs;
  sigar_loadavg_t avg;
  DBusGTypeSpecializedAppendContext appendctx;
  gpointer ret;
  GType gtype;
  GValue key_value = {0,};
  GValue value_value = {0,};

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
    // 1/5/15 minute load average - map
    host_get_load_averages(&avg);

    gtype = G_VALUE_TYPE (value);
    ret = dbus_g_type_specialized_construct (gtype);
    g_value_set_boxed_take_ownership (value, ret);

    dbus_g_type_specialized_init_append (value, &appendctx);

    g_value_init (&key_value, G_TYPE_STRING);
    g_value_init (&value_value, G_TYPE_DOUBLE);

    g_value_set_static_string(&key_value, "1");
    g_value_set_double(&value_value, avg.loadavg[0]);
    dbus_g_type_specialized_map_append (&appendctx, &key_value, &value_value);

    g_value_set_static_string(&key_value, "5");
    g_value_set_double(&value_value, avg.loadavg[1]);
    dbus_g_type_specialized_map_append (&appendctx, &key_value, &value_value);

    g_value_set_static_string(&key_value, "15");
    g_value_set_double(&value_value, avg.loadavg[2]);
    dbus_g_type_specialized_map_append (&appendctx, &key_value, &value_value);
    break;
  case PROP_PROCESS_STATISTICS:
    // Process statistics is type map string -> int
    host_get_processes(&procs);

    gtype = G_VALUE_TYPE (value);
    ret = dbus_g_type_specialized_construct (gtype);
    g_value_set_boxed_take_ownership (value, ret);

    dbus_g_type_specialized_init_append (value, &appendctx);

    g_value_init (&key_value, G_TYPE_STRING);
    g_value_init (&value_value, G_TYPE_INT);

    g_value_set_static_string(&key_value, "total");
    g_value_set_int(&value_value, procs.total);
    dbus_g_type_specialized_map_append (&appendctx, &key_value, &value_value);

    g_value_set_static_string(&key_value, "idle");
    g_value_set_int(&value_value, procs.idle);
    dbus_g_type_specialized_map_append (&appendctx, &key_value, &value_value);

    g_value_set_static_string(&key_value, "zombie");
    g_value_set_int(&value_value, procs.zombie);
    dbus_g_type_specialized_map_append (&appendctx, &key_value, &value_value);

    g_value_set_static_string(&key_value, "running");
    g_value_set_int(&value_value, procs.running);
    dbus_g_type_specialized_map_append (&appendctx, &key_value, &value_value);

    g_value_set_static_string(&key_value, "stopped");
    g_value_set_int(&value_value, procs.stopped);
    dbus_g_type_specialized_map_append (&appendctx, &key_value, &value_value);

    g_value_set_static_string(&key_value, "sleeping");
    g_value_set_int(&value_value, procs.sleeping);
    dbus_g_type_specialized_map_append (&appendctx, &key_value, &value_value);
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
  GParamSpec *pspec = NULL;
  GType value_type;

  g_type_class_add_private(matahari_class, sizeof (MatahariPrivate));

  gobject_class->set_property = matahari_set_property;
  gobject_class->get_property = matahari_get_property;

  int i;
  for (i = 0; properties_Host[i].name != NULL; i++)
  {
    if (!get_paramspec_from_property(properties_Host[i], &pspec))
    {
        // Type is map - type of parameters must be added manually!
        if (properties_Host[i].type == 'e')
        {
            switch (properties_Host[i].prop)
            {
                case PROP_LOAD:
                    value_type = G_TYPE_DOUBLE;
                    break;
                case PROP_PROCESS_STATISTICS:
                    value_type = G_TYPE_INT;
                    break;
                default:
                    g_printerr("Type of property %s is map of unknown types\n", properties_Host[i].name);
                    value_type = G_TYPE_VALUE;
            }

            pspec = g_param_spec_boxed(properties_Host[i].name,
                                       properties_Host[i].nick,
                                       properties_Host[i].desc,
                                       dbus_g_type_get_map("GHashTable", G_TYPE_STRING, value_type),
                                       properties_Host[i].flags);
        }
        else
        {
            g_printerr("Unknown type: %c\n", properties_Host[i].type);
            pspec = NULL;
        }
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
  g_type_init();
  return run_dbus_server(MATAHARI_TYPE, HOST_BUS_NAME, HOST_OBJECT_PATH);
}
