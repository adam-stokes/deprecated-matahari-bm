
#include "mh_dbus_common.h"

#include <polkit/polkit.h>

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
  if (!res)
  {
    g_set_error(error, MATAHARI_ERROR, MATAHARI_AUTHENTICATION_ERROR, "You are not authorized for specified action");
    g_printerr("Caller is not authorized for action %s\n", action);
  }
  return res;
}
