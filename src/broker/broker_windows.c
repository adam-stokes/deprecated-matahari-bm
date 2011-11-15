/* Copyright (C) 2011 Red Hat, Inc.
 * Written by Zane Bitter <zbitter@redhat.com>
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

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include "broker_federation.h"
#include "broker_os.h"


int broker_os_start_broker(char * const args[])
{
    gchar *cmd[] = {
        "qpidd",
        "--auth",
        "no",
        "--port",
        "49000",
        "--daemon",
        NULL,
    };
    gboolean ret;
    GError *error = NULL;
    gint res = 0;

    ret = g_spawn_sync(NULL, cmd, NULL, G_SPAWN_SEARCH_PATH,
            NULL, NULL, NULL, NULL, &res, &error);

    if (ret == FALSE) {
        g_error_free(error);
        return MH_RES_BACKEND_ERROR;
    }

    if (res > 0)
        return MH_RES_SUCCESS;
    else
        return MH_RES_BACKEND_ERROR;

}

int broker_os_add_qpid_route_link(const char *local, const char *remote)
{
    return 0;
}

int broker_os_add_qpid_route(const struct mh_qpid_route *route)
{
    return 0;
}

