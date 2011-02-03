
#include "mh_dbus_common.h"

#include <glib/gi18n.h>

#include <polkit/polkit.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <dbus/dbus-glib-bindings.h>

GQuark
matahari_error_quark (void)
{
  return g_quark_from_static_string ("matahari-error-quark");
}


gboolean
check_authorization(const gchar *action, GError** error, DBusGMethodInvocation *context)
{
  if (context == NULL)
  {
    g_printerr("Context is not set!\n");
    return FALSE;
  }
  GError *err = NULL;
  PolkitAuthorizationResult *result;
  PolkitSubject *subject = polkit_system_bus_name_new(dbus_g_method_get_sender(context));
  PolkitAuthority *authority = polkit_authority_get_sync(NULL, &err);
  if (err != NULL)
  {
    g_printerr("Error in obtaining authority: %s", err->message);
    g_propagate_error(error, err);
    g_error_free(err);
    return FALSE;
  }

  result = polkit_authority_check_authorization_sync(authority, subject, action, NULL, POLKIT_CHECK_AUTHORIZATION_FLAGS_ALLOW_USER_INTERACTION, NULL, &err);
  if (err != NULL)
  {
    g_printerr("Error in checking authorization: %s", err->message);
    g_propagate_error(error, err);
    g_error_free(err);
    return FALSE;
  }
  gboolean res = polkit_authorization_result_get_is_authorized(result);
  g_object_unref(subject);
  g_object_unref(result);
  g_object_unref(authority);
  if (!res)
  {
    g_set_error(error, MATAHARI_ERROR, MATAHARI_AUTHENTICATION_ERROR, "You are not authorized for specified action");
    g_printerr("Caller is not authorized for action %s\n", action);
  }
  return res;
}

gboolean
get_paramspec_from_property(Property prop, GParamSpec** pspec)
{
    switch (prop.type)
    {
        case 's':
            *pspec = g_param_spec_string(prop.name,
                                        prop.nick,
                                        prop.desc,
                                        NULL,
                                        prop.flags);
            break;
        case 'b':
            *pspec = g_param_spec_boolean(prop.name,
                                         prop.nick,
                                         prop.desc,
                                         FALSE,
                                         prop.flags);
            break;
        case 'n':
        case 'i':
            *pspec = g_param_spec_int(prop.name,
                                     prop.nick,
                                     prop.desc,
                                     G_MININT, G_MAXINT, 0,
                                     prop.flags);
            break;
        case 'x':
            *pspec = g_param_spec_int64(prop.name,
                                       prop.nick,
                                       prop.desc,
                                       G_MININT64, G_MAXINT64, 0,
                                       prop.flags);

            break;
        case 'y':
        case 'q':
        case 'u':
            *pspec = g_param_spec_uint(prop.name,
                                      prop.nick,
                                      prop.desc,
                                      0, G_MAXUINT, 0,
                                      prop.flags);
            break;
        case 't':
            *pspec = g_param_spec_uint64(prop.name,
                                        prop.nick,
                                        prop.desc,
                                        0, G_MAXUINT64, 0,
                                        prop.flags);

            break;
        case 'd':
            *pspec = g_param_spec_double(prop.name,
                                        prop.nick,
                                        prop.desc,
                                        -G_MAXDOUBLE, G_MAXDOUBLE, 0,
                                        prop.flags);
            break;
        default:
            return FALSE;
    }
    return TRUE;

}

int
run_dbus_server(GType matahari_type, char *bus_name, char *object_path)
{
  GMainLoop* loop = NULL;
  DBusGConnection *connection = NULL;
  GError *error = NULL;
  GObject *obj = NULL;
  DBusGProxy *driver_proxy = NULL;
  guint32 request_name_ret;

  loop = g_main_loop_new(NULL, FALSE);

  /* Obtain a connection to the system bus */
  connection = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);
  if (!connection)
    {
      g_printerr(_("Failed to open connection to bus: %s\n"), error->message);
      g_error_free(error);
      return 1;
    }

  obj = g_object_new(matahari_type, NULL);
  dbus_g_connection_register_g_object(connection, object_path, obj);

  driver_proxy = dbus_g_proxy_new_for_name(connection, DBUS_SERVICE_DBUS,
      DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS);

  if (!org_freedesktop_DBus_request_name(driver_proxy, bus_name, 0,
      &request_name_ret, &error))
    {
      g_printerr(_("Failed to get name: %s\n"), error->message);
      g_error_free(error);
      return 1;
    }

  switch (request_name_ret)
    {
  case DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER:
    break;
  case DBUS_REQUEST_NAME_REPLY_IN_QUEUE:
    g_printerr(
        _("Looks like another server of this type is already running: Reply in queue\n"));
    return 1;
  case DBUS_REQUEST_NAME_REPLY_EXISTS:
    g_printerr(
        _("Looks like another server of this type is already running: Reply exists\n"));
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
