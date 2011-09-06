/* utilities_linux.c - Copyright (C) 2011, Red Hat, Inc.
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

#include "config.h"

#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "matahari/logging.h"
#include "utilities_private.h"

const char *
mh_os_dnsdomainname(void)
{
    static char *dnsdomainname = NULL;
    int res;
    char *c;
    struct addrinfo *ai = NULL;
	struct addrinfo hints = {
        hints.ai_socktype = SOCK_DGRAM,
        hints.ai_flags = AI_CANONNAME,
    };

    /**
     * \todo It would be good to eventually add this as a feature in sigar
     * since we use that for most related things.
     */

    if (dnsdomainname) {
        return dnsdomainname;
    }

    if ((res = getaddrinfo(mh_hostname(), NULL, &hints, &ai))) {
        mh_notice("Unable to determine dnsdomainname: (%d) %s\n", errno, strerror(errno));
        return "";
    }

    if (!ai) {
        return "";
    }

	if ((c = strchr(ai->ai_canonname, '.'))) {
        dnsdomainname = strdup(++c);
    }

    return dnsdomainname ? dnsdomainname : "";
}

const char *
mh_os_uuid(void)
{
    static char *uuid = NULL;

    if (uuid == NULL && g_file_test("/etc/machine-id", G_FILE_TEST_EXISTS)) {
        uuid = mh_file_first_line("/etc/machine-id");
    }
    if (uuid == NULL && g_file_test("/var/lib/dbus/machine-id", G_FILE_TEST_EXISTS)) {
        /* For pre-systemd machines */
        uuid = mh_file_first_line("/var/lib/dbus/machine-id");
    }

    if(uuid) {
        mh_trace("Got uuid: %s", uuid);
    }
    return uuid;
}
