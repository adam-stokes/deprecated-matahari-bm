/*
 * network_linux.c: linux network functions
 *
 * Copyright (C) 2010 Red Hat Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 *
 * Author: Adam Stokes <astokes@fedoraproject.org>
 */

#include "matahari/network.h"
#include <string.h>

void
network_os_start(const char *iface)
{
    gboolean ret;
    GError *error = NULL;
    gchar *argv[] = {
        "/sbin/ifup", (gchar *) iface, NULL
    };

    ret = g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL,
                        &error);
    if (ret == FALSE) {
        g_error_free(error);
    }
}

void
network_os_stop(const char *iface)
{
    gboolean ret;
    GError *error = NULL;
    gchar *argv[] = {
        "/sbin/ifdown", (gchar *) iface, NULL
    };

    ret = g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL,
                        &error);
    if (ret == FALSE) {
        g_error_free(error);
    }
}
