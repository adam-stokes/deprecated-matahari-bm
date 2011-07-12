/* postboot.c - Copyright (C) 2011 Red Hat, Inc.
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

#ifndef WIN32
#include "config.h"
#include <stddef.h>
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include "matahari/logging.h"
#include "matahari/utilities.h"
#include "matahari/postboot.h"


MH_TRACE_INIT_DATA(mh_postboot);


static int puppet(const char *uri) {
    int ret;
    char *cmd = NULL;

    // Call puppet manifest locally
    cmd = asprintf("puppet %s", uri);
    ret = system(cmd);
    free(cmd);
    return ret;
}

void mh_unconfigure()
{
    if(g_file_test(mh_filename, G_FILE_TEST_EXISTS)) {
        g_remove(mh_filename);
    }
}

void mh_configure(const char *uri, int type)
{
    int ret;
    char *buf = NULL;

    switch(type) {
        case PUPPET:
            ret = puppet(uri);
            if(ret == 0) {
                buf = strdup("Puppet configured.");
                if(!g_file_set_contents(mh_filename, buf, -1, NULL)) {
                    free(buf);
                    goto fail;
                }
            }
            break;
        default:
            break;
    }
fail:
    mh_unconfigure();
}
