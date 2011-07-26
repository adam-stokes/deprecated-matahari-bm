/* sysconfig.c - Copyright (C) 2011 Red Hat, Inc.
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

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gprintf.h>

#include "matahari/logging.h"
#include "matahari/sysconfig.h"
#include "matahari/utilities.h"
#include "sysconfig_private.h"

MH_TRACE_INIT_DATA(mh_sysconfig);

#ifdef WIN32
static const char keys_dir[] = "c:\\";
#else
static const char keys_dir[] = "/var/lib/matahari/";
#endif

static gboolean
set_key(const char *key)
{
    char key_file[PATH_MAX];
    const char contents[] = "ok";

    g_snprintf(key_file, sizeof(key_file), "%s%s", keys_dir, key);
    if (!g_file_set_contents(key_file, contents, strlen(contents), NULL)) {
        mh_err("Could not set file %s", key_file);
        return FALSE;
    }
    return TRUE;
}

static gboolean
get_key(const char *key)
{
    char key_file[PATH_MAX];
    char *contents;
    size_t length;

    g_snprintf(key_file, sizeof(key_file), "%s%s", keys_dir, key);
    if (g_file_test(key_file, G_FILE_TEST_EXISTS)) {
        g_file_get_contents(key_file, &contents, &length, NULL);
        if ((strcmp(contents, "ok")) == 0) {
        	free(contents);
            return TRUE;
        }
    }
    return FALSE;
}

gboolean
mh_sysconfig_set_configured(const char *key)
{
    if (!set_key(key)) {
        return FALSE;
    }
    return TRUE;
}

gboolean
mh_sysconfig_is_configured(const char *key)
{
    if (!get_key(key)) {
        return FALSE;
    }
    return TRUE;
}

int
mh_sysconfig_run_uri(const char *uri, uint32_t flags, const char *scheme,
        const char *key) {
    return sysconfig_os_run_uri(uri, flags, scheme, key);
}

int
mh_sysconfig_run_string(const char *string, uint32_t flags, const char *scheme,
        const char *key) {
    return sysconfig_os_run_string(string, flags, scheme, key);
}

const char *
mh_sysconfig_query(const char *query, uint32_t flags, const char *scheme) {
    return sysconfig_os_query(query, flags, scheme);
}
