/* sysconfig_windows.c - Copyright (C) 2011 Red Hat, Inc.
 * Written by Adam Stokes <astokes@fedoraproject.org>
 * Written by Russell Bryant <rbryant@redhat.com>
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
run_regedit(char *registry_file)
{
    gchar *cmd[] = {
        "REGEDIT",
        "/S",
        registry_file,
        NULL,
    };
    gboolean ret;
    GError *error = NULL;
    gint res = 0;

    ret = g_spawn_sync(NULL, cmd, NULL, G_SPAWN_SEARCH_PATH,
            NULL, NULL, NULL, &res, &error);

    if (ret == FALSE) {
        g_error_free(error);
        return -1;
    }

    return res;
}

int
sysconfig_os_run_uri(const char *uri, uint32_t flags, const char *scheme,
        const char *key, mh_sysconfig_result_cb result_cb, void *cb_data)
{
    return -1;
}

int
sysconfig_os_run_string(const char *string, uint32_t flags, const char *scheme,
        const char *key, mh_sysconfig_result_cb result_cb, void *cb_data);
{
    if (!strcasecmp(scheme, "registry")) {
        char filename[PATH_MAX];
        int res;

        g_snprintf(filename, sizeof(filename), "%s\\%s.REG", g_getenv("TEMP"), key);
        g_file_set_contents(filename, data, strlen(data), NULL);

        res = run_regedit(filename);

        if (!res) {
            result_cb(cb_data, res);
        }

        return res;
    }

    return -1;
}

const char *
sysconfig_os_query(const char *query, uint32_t flags, const char *scheme)
{
    const char *data = NULL;
    return data;
}
