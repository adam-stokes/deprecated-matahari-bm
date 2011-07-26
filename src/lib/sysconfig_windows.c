/* sysconfig_windows.c - Copyright (C) 2011 Red Hat, Inc.
 * Written by Adam Stokes <astokes@fedoraproject.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <windows.h>
#include <glib.h>

#include "matahari/logging.h"
#include "matahari/sysconfig.h"
#include "sysconfig_private.h"

MH_TRACE_INIT_DATA(mh_sysconfig);

static int
sysconfig_os_run_regedit(char *registry_file)
{
    gchar *cmd[4];
    gboolean ret;
    GError *error = NULL;

    cmd[0] = "REGEDIT";
    cmd[1] = "/S";
    cmd[2] = registry_file;
    cmd[3] = NULL;

    ret = g_spawn_async(NULL, cmd, NULL, G_SPAWN_SEARCH_PATH,
            NULL, NULL, NULL, &error);
    if (ret == FALSE) {
        g_error_free(error);
        return -1;
    }
    return 0;
}

int
sysconfig_os_run_uri(const char *uri, uint32_t flags, const char *scheme,
        const char *key)
{
    return 0;
}

int
sysconfig_os_run_string(const char *data, uint32_t flags, const char *scheme,
        const char *key)
{
    char filename[PATH_MAX];
    g_snprintf(filename, sizeof(filename), "%s\\%s.REG", g_getenv("TEMP"), key);
    if (strcasecmp(scheme, "registry") == 0 ) {
            g_file_set_contents(filename, data, strlen(data), NULL);
            return sysconfig_os_run_regedit(filename);
    }
    return 0;
}

const char *
sysconfig_os_query(const char *query, uint32_t flags, const char *scheme)
{
    const char *data = NULL;
    return data;
}
