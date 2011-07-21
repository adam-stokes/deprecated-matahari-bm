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
#include <string.h>
#include <fcntl.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gprintf.h>

#include "matahari/logging.h"
#include "matahari/sysconfig.h"
#include "matahari/utilities.h"

MH_TRACE_INIT_DATA(mh_sysconfig);

static const char configured_fn[] = "/var/lib/matahari/.is_configured";

static char **
string_to_array(const char *contents)
{
    char **keys = NULL;
    keys = g_strsplit(contents, "\n", 0);
    return keys;
}

static gboolean
set_key(const char *key)
{
    char *contents = NULL;
    char *contents_appended = NULL;
    char **keys_array = NULL;
    size_t length;

    if (!g_file_test(configured_fn, G_FILE_TEST_EXISTS)) {
        if (!g_file_set_contents(configured_fn, key, -1, NULL)) {
            mh_err("Could not read file %s", configured_fn);
            return FALSE;
        }
    } else {
        if (g_file_get_contents(configured_fn, &contents, &length, NULL)) {
            int i = 0;
            keys_array = string_to_array(contents);
            while (keys_array[i] != NULL) {
                if ((strcasecmp(key, keys_array[i])) == 0) {
                    return TRUE;
                }
                i++;
            }
            contents_appended = g_strconcat(contents, "\n", key, NULL);
            g_file_set_contents(configured_fn, contents_appended, -1, NULL);
            g_strfreev(keys_array);
        } else {
            mh_err("Unable to write to keys file");
            return FALSE;
        }
    }
    return TRUE;
}

static gboolean
get_key(const char *key)
{
    size_t length;
    int i = 0;
    char *contents = NULL;
    char **keys_array = NULL;
    if (g_file_test(configured_fn, G_FILE_TEST_EXISTS)) {
        g_file_get_contents(configured_fn, &contents, &length, NULL);
        keys_array = string_to_array(contents);
        while (keys_array[i] != NULL) {
            if ((strcasecmp(key, keys_array[i])) == 0) {
                g_strfreev(keys_array);
                return TRUE;
            }
            i++;
        }
    }
    g_strfreev(keys_array);
    return FALSE;
}

gboolean
mh_set_configured(const char *key)
{
    if (!set_key(key)) {
        return FALSE;
    }
    return TRUE;
}

gboolean
mh_is_configured(uint32_t flags, const char *key)
{
    if (flags & MH_SYSCONFIG_FLAG_FORCE) {
        return FALSE;
    }

    if (!get_key(key)) {
        return FALSE;
    }
    return TRUE;
}

