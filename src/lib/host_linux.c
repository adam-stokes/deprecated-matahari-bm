/* host_linux.c - Copyright (C) 2010 Red Hat, Inc.
 * Written by Darryl L. Pierce <dpierce@redhat.com>
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <sys/reboot.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>

#include <linux/reboot.h>

#include <pcre.h>

#include "matahari/logging.h"
#include "matahari/host.h"
#include "host_private.h"

const char *
host_os_get_uuid()
{
    static char * uuid = NULL;

    if(uuid == NULL) {
	FILE *input = fopen("/var/lib/dbus/machine-id", "r");
	char *buffer = NULL;
	int chunk = 512, data_length = 0, read_chars = 0;
	do {
	    buffer = realloc(buffer, chunk + data_length + 1);
	    read_chars = fread(buffer + data_length, 1, chunk, input);
	    data_length += read_chars;
	} while (read_chars > 0);

	if(data_length == 0) {
	    mh_warn("Could not read from /var/lib/dbus/machine-id");
	} else {
	    int lpc = 0;
	    for (; lpc < data_length; lpc++) {
		switch(buffer[lpc]) {
		    case '\0':
		    case '\n':
			uuid = malloc(lpc);
			snprintf(uuid, lpc-1, "%s", buffer);
		}
	    }
	}
	fclose(input);
	free(buffer);
    }
    
    return uuid;
}

void
host_os_reboot(void)
{
    reboot(LINUX_REBOOT_CMD_RESTART);
}

void
host_os_shutdown(void)
{
    reboot(LINUX_REBOOT_CMD_HALT);
}

    