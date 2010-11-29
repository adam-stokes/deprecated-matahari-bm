/* host.cpp - Copyright (C) 2010 Red Hat, Inc.
 * Written by Darryl L. Pierce <dpierce@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.  A copy of the GNU General Public License is
 * also available at http://www.gnu.org/copyleft/gpl.html.
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

#ifdef HAVE_LIBVIRT1
#include <libvirt/libvirt.h>
#endif

#include <pcre.h>
#include <libudev.h>

#include "matahari/logging.h"
#include "matahari/host.h"
#include "host_private.h"

const char *
host_os_get_uuid()
{
    static char * uuid = NULL;

    if(uuid != NULL) {
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
host_os_get_hostname(void)
{
    static char *hostname = NULL;
    if(hostname == NULL) {
	struct utsname details;

	if(!uname(&details)) {
	    hostname = strdup(details.nodename);
	}
    }

    return hostname;
}

const char *
host_os_get_operating_system(void)
{
    static char *operating_system = NULL;

    if(operating_system == NULL) {
	struct utsname details;
	if(!uname(&details)) {
	    int len = 4 + strlen(details.sysname) + strlen(details.release);
	    operating_system = malloc(len);
	    sprintf(operating_system, "%s (%s)", details.sysname, details.release);
	}
    }

    return operating_system;
}

const char *
host_os_get_hypervisor(void)
{
    static char * hypervisor;

    if(hypervisor == NULL) {

#ifdef HAVE_LIBVIRT1
	virConnectPtr lvconn = virConnectOpenReadOnly(NULL);
      
	if(lvconn) {
	    hypervisor = strdup(virConnectGetType(lvconn));
	    virConnectClose(lvconn);
	}
#endif
      
    }
    return hypervisor;
}

const char *
host_os_get_architecture(void)
{
    static char *architecture;

    if(architecture == NULL) {
	struct utsname details;

	if(!uname(&details)) {
	    architecture = strdup(details.machine);
	}
    }

    return architecture;
}

/* TODO: Replace with sigar calls */

unsigned int
host_os_get_memory(void)
{
  static unsigned int memory = 0;

  if(!memory) {
      struct sysinfo sysinf;
      if(!sysinfo(&sysinf)) {
	  memory = sysinf.totalram / 1024L;
      }
  }

  return memory;
}

/* TODO: Replace with sigar calls */
void
host_os_get_load_averages(double *one, double *five, double *fifteen)
{
    size_t chunk = 512;
    size_t read_chars = 0;
    size_t data_length = 0;
    
    char *buffer = NULL;
    FILE *input = NULL;

    *one = 0.0;
    *five = 0.0;
    *fifteen = 0.0;
    
    input = fopen("/proc/loadavg", "r");

    do {
	buffer = realloc(buffer, chunk + data_length + 1);
	read_chars = fread(buffer + data_length, 1, chunk, input);
	data_length += read_chars;
    } while (read_chars > 0);

    if(data_length != 0) {
	sscanf(buffer, "%f %f %f", one, five, fifteen);
    }
    fclose(input);
    free(buffer);
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

void
host_os_get_cpu_details(void)
{
    size_t chunk = 512;
    size_t read_chars = 0;
    size_t data_length = 0;
    
    char *buffer = NULL;
    FILE *input = NULL;
    
    if(cpuinfo.initialized) return;
    cpuinfo.initialized = 1;

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
	    mh_err("Unable to compile regular expression.");
	    goto done;
	}

	for(lpc = 0; lpc < data_length; lpc++) {
	    switch(offset) {
		case '\n':
		case '\0':
		    match = pcre_exec(regex, NULL, buffer+offset, lpc-offset,
				      0, PCRE_NOTEMPTY, found, 9);
		    
		    if(match == expected) {
			char *name = NULL;
			char *value = NULL;

			name = malloc(1 + found[3] - found[2]);
			snprintf(name, found[3] - found[2], "%s", buffer + offset + found[2]);
			
			value = malloc(1 + found[5] - found[4]);
			snprintf(value, found[5] - found[4], "%s", buffer + offset + found[4]);
		
			if (name == "processor") {
			    cpuinfo.cpus++;
			    
			} else if (name == "cpu cores")  {
			    cpuinfo.cores += atoi(value);
			    
			} else if (name == "model name") {
			    cpuinfo.model = strdup(value);
			    
			} else if (name == "flags") {
			    /* if the cpuflags contain "lm" then it's a 64 bit CPU
			     * http://www.brandonhutchinson.com/Understanding_proc_cpuinfo.html
			     */
			    if(strstr(value, " lm ")) {
				cpuinfo.wordsize = 64;
			    } else {
				cpuinfo.wordsize = 32;
			    }
			}
			free(value);
			free(name);
		    }
		    offset = lpc+1;
	    }
	}
    }
  done:
    fclose(input);
    free(buffer);
}