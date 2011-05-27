/* mh_agent_config.c - Copyright (C) 2011 Red Hat, Inc.
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
#include <glib.h>
#include "matahari/logging.h"
#include "matahari/mh_agent_config.h"


MH_TRACE_INIT_DATA(mh_config);

static const char *filename = "/var/lib/matahari/.mh_configured";

uint32_t mh_is_configured()
{
    int config_file_exist = 0;
    
    if(g_file_test(filename, G_FILE_TEST_EXISTS)) {
        config_file_exist = 1;
    }
    return config_file_exist;
}

void mh_configure(const char *uri)
{
    if(!g_file_set_contents(filename, uri, -1, NULL)) {
        mh_log(LOG_DEBUG, "Unable to create file.");
    }
}
