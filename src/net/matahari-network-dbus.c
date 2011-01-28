/*
 * matahari-network-dbus.c
 *
 * Copyright (C) 2010 Red Hat, Inc.
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

#include <stdio.h>
#include <string.h>

#include <sigar.h>

#include "mh_dbus_common.h"

/* GObject class definition */
#include "mh_gobject_class.h"

/* Network methods */
#include "matahari/network.h"
#include "matahari/utilities.h"

/* Generated properties list */
#include "matahari-network-dbus-properties.h"

/* DBus names */
#define NETWORK_BUS_NAME "org.matahariproject.Network"
#define NETWORK_OBJECT_PATH "/org/matahariproject/Network"
#define NETWORK_INTERFACE_NAME "org.matahariproject.Network"
#define DBUS_PROPERTY_INTERAFACE_NAME "org.freedesktop.DBus.Properties"

/* Private struct in Matahari class */
#define MATAHARI_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MATAHARI_TYPE, MatahariPrivate))

struct _MatahariPrivate
{
  int x;
};

static int interface_status(const char *iface)
{
  uint64_t flags = 0;
  if(iface == NULL)
    return 3;
  
  network_status(iface, &flags);

  if(flags & SIGAR_IFF_UP)
    return 0;
  return 1; /* Inactive */
}

/* Dbus methods */

gboolean
Network_list(Matahari *matahari, DBusGMethodInvocation *context)
{
  GError* error = NULL;
  GList *interface_list;
  GList *plist;
  sigar_net_interface_config_t *ifconfig;
  
  if (!check_authorization(NETWORK_BUS_NAME ".list", &error, context))
  {
    dbus_g_method_return_error(context, error);
    return FALSE;
  }
  
  // Get the list of interfaces
  interface_list = network_get_interfaces();
  
  // Alloc array for list of interface names
  char **list = g_new(char *, g_list_length(interface_list) + 1);  

  // Convert list of interfaces to the array of names
  int i = 0;
  for(plist = g_list_first(interface_list); plist; plist = g_list_next(plist))
  {
    ifconfig = (sigar_net_interface_config_t *)plist->data;
    list[i++] = strdup(ifconfig->name);
  }
  list[i] = NULL; // Sentinel

  dbus_g_method_return(context, list);
  g_strfreev(list);
  g_list_free(interface_list);
  return TRUE;
}

gboolean
Network_start(Matahari *matahari, const char *iface, DBusGMethodInvocation *context)
{
  GError* error = NULL;
  if (!check_authorization(NETWORK_BUS_NAME ".start", &error, context))
  {
    dbus_g_method_return_error(context, error);
    return FALSE;
  }
  
  int status = interface_status(iface);
  if (status != 1)
  {
    network_start(iface);
    status = interface_status(iface);
  }
  dbus_g_method_return(context, status);
  return TRUE;
}

gboolean
Network_stop(Matahari *matahari, const char *iface, DBusGMethodInvocation *context)
{
  GError* error = NULL;
  if (!check_authorization(NETWORK_BUS_NAME ".stop", &error, context))
  {
    dbus_g_method_return_error(context, error);
    return FALSE;
  }
  
  int status = interface_status(iface);
  if (status == 0)
  {
    network_stop(iface);
    status = interface_status(iface);
  }
  dbus_g_method_return(context, status);
  return TRUE;
}

gboolean
Network_status(Matahari *matahari, const char *iface, DBusGMethodInvocation *context)
{
  GError* error = NULL;
  if (!check_authorization(NETWORK_BUS_NAME ".status", &error, context))
  {
    dbus_g_method_return_error(context, error);
    return FALSE;
  }

  dbus_g_method_return(context, interface_status(iface));
  return TRUE;
}

gboolean
Network_get_ip_address(Matahari *matahari, const char *iface, DBusGMethodInvocation *context)
{
  GError* error = NULL;
  if (!check_authorization(NETWORK_BUS_NAME ".get_ip_address", &error, context))
  {
    dbus_g_method_return_error(context, error);
    return FALSE;
  }
  
  dbus_g_method_return(context, g_strdup(network_get_ip_address(iface)));
  return TRUE;
}

gboolean
Network_get_mac_address(Matahari *matahari, const char *iface, DBusGMethodInvocation *context)
{
  GError* error = NULL;
  if (!check_authorization(NETWORK_BUS_NAME ".get_mac_address", &error, context))
  {
    dbus_g_method_return_error(context, error);
    return FALSE;
  }
  
  dbus_g_method_return(context, g_strdup(network_get_mac_address(iface)));
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

/* Generated dbus stuff for network
 * MUST be after declaration of user defined functions.
 */
#include "matahari-network-dbus-glue.h"

//TODO: Properties get/set
static void
matahari_set_property(GObject *object, guint property_id, const GValue *value,
    GParamSpec *pspec)
{
  // We don't have writable other property...
  G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void
matahari_get_property(GObject *object, guint property_id, GValue *value,
    GParamSpec *pspec)
{
  switch (property_id)
    {
  case PROP_HOSTNAME:
    g_value_set_string (value, get_hostname());
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

  g_type_class_add_private(matahari_class, sizeof (MatahariPrivate));

  gobject_class->set_property = matahari_set_property;
  gobject_class->get_property = matahari_get_property;

  int i;
  for (i = 0; properties_Network[i].name != NULL; i++)
  {
    if (!get_paramspec_from_property(properties_Network[i], &pspec))
    {
        g_printerr("Unknown type: %c\n", properties_Network[i].type);
        pspec = NULL;
    }
    if (pspec)
        g_object_class_install_property(gobject_class, properties_Network[i].prop, pspec);
  }

  dbus_g_object_type_install_info(MATAHARI_TYPE, &dbus_glib_matahari_object_info);
}

/* Instance init */
static void
matahari_init(Matahari *matahari)
{
  MatahariPrivate *priv;
  matahari->priv = priv = MATAHARI_GET_PRIVATE(matahari);
}

int
main(int argc, char** argv)
{
  g_type_init();
  return run_dbus_server(MATAHARI_TYPE, NETWORK_BUS_NAME, NETWORK_OBJECT_PATH);
}
