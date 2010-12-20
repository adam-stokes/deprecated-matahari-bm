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
host_os_get_uuid(void)
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

const char *
host_os_get_cpu_flags(void)
{
    static char *flags = NULL;
    
    size_t chunk = 512;
    size_t read_chars = 0;
    size_t data_length = 0;
    
    char *buffer = NULL;
    FILE *input = NULL;

    if(flags) {
	return flags;
    }
    
    input = fopen("/proc/cpuinfo", "r");

    do {
	buffer = realloc(buffer, chunk + data_length + 1);
	read_chars = fread(buffer + data_length, 1, chunk, input);
	data_length += read_chars;
    } while (read_chars > 0);

    if(data_length == 0) {
	mh_warn("Could not read from /proc/cpuinfo");

    } else {
	const char *regexstr = "(.*\\S)\\s*:\\s*(\\S.*)";
	int expected = 3;
	int found[9];
	const char* pcre_error;
	int pcre_error_offset;
	pcre* regex;
	int offset = 0, lpc = 0, match = 0;
       
	buffer[data_length] = '\0';
	regex = pcre_compile(regexstr, 0, &pcre_error, &pcre_error_offset, NULL);
	if(!regex) {
	    mh_err("Unable to compile regular expression '%s' at offset %d: %s",
		   regexstr, pcre_error_offset, pcre_error);
	    goto done;
	}

	for(lpc = 0; lpc < data_length; lpc++) {
           
	    switch(buffer[lpc]) {
		case '\n':
		case '\0':
		    match = pcre_exec(regex, NULL, buffer+offset, lpc-offset,
				      0, PCRE_NOTEMPTY, found, 9);
                   
		    if(match == expected) {
			char *name = malloc(2 + found[3] - found[2]);
			snprintf(name, 1 + found[3] - found[2], "%s", buffer + offset + found[2]);
                       
			if (strcmp(name, "flags") == 0) {
			    flags = malloc(2 + found[5] - found[4]);
			    snprintf(flags, 1 + found[5] - found[4], "%s", buffer + offset + found[4]);
			    free(name);
			    goto done;
			}
			free(name);
		    }
		    offset = lpc+1;
	    }
	}
    }
    
  done:
    fclose(input);
    free(buffer);

    if(flags == NULL) {
	flags = strdup("unknown");
    }
    
    return flags;
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

    
