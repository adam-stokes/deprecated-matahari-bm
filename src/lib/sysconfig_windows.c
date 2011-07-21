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

#include <glib.h>

#include "matahari/logging.h"
#include "matahari/sysconfig.h"

MH_TRACE_INIT_DATA(mh_sysconfig);

int
mh_sysconfig_run_uri(const char *uri, uint32_t flags, const char *scheme,
        const char *key)
{

    return 0;
}

int
mh_sysconfig_run_string(const char *data, uint32_t flags, const char *scheme,
        const char *key)
{

    return 0;
}

const char *
mh_sysconfig_query(const char *query, uint32_t flags, const char *scheme)
{
    const char *data = NULL;
    return data;
}
