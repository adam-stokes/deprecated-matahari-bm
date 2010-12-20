/* host.c - Copyright (C) 2010 Red Hat, Inc.
 * Written by Andrew Beekhof <andrew@beekhof.net>
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
#endif

#ifdef __linux__
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#endif

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <glib.h>
#include <glib/gprintf.h>
#include "matahari/host.h"
#include "matahari/logging.h"
#include "host_private.h"

#include <sigar.h>
#include <sigar_private.h>
#include <sigar_format.h>

MH_TRACE_INIT_DATA(mh_host);

host_init_t host_init = { NULL, FALSE };
cpuinfo_t cpuinfo = { NULL, 0, 0 };

static void
init(void)
{
    if(host_init.sigar_init) return;
    sigar_open(&host_init.sigar);
    host_init.sigar_init = TRUE;
}

const char *
host_get_uuid(void)
{
    return host_os_get_uuid();
}

const char *
host_get_hostname(void)
{
    init();
    sigar_net_info_t netinfo;
    char *host_name = NULL;

    if(host_name == NULL) {
        sigar_net_info_get(host_init.sigar, &netinfo);
        host_name = g_strdup(netinfo.host_name);
    }
    return host_name;
}

const char *
host_get_operating_system(void)
{
    init();
    sigar_sys_info_t sysinfo;
    char *operating_system = NULL;

    if(operating_system == NULL) {
        sigar_sys_info_get(host_init.sigar, &sysinfo);
        operating_system = g_strdup_printf("%s (%s)", sysinfo.vendor_name, sysinfo.version);
    }
    return operating_system;
}

int
host_get_cpu_wordsize(void)
{
    return (int)(CHAR_BIT * sizeof(size_t));
}

const char *
host_get_architecture(void)
{
    init();
    sigar_sys_info_t sysinfo;
    char *arch = NULL;

    if(arch == NULL) {
        sigar_sys_info_get(host_init.sigar, &sysinfo);
        arch = g_strdup(sysinfo.arch);
    }
    return arch;
}

void
host_reboot(void)
{
    host_os_reboot();
}

void
host_shutdown(void)
{
    host_os_shutdown();
}

const char*
host_get_cpu_model(void)
{
    host_get_cpu_details();
    return cpuinfo.model;
}

const char *
host_get_cpu_flags(void)
{
    return host_os_get_cpu_flags();
}

int
host_get_cpu_count(void)
{
    host_get_cpu_details();
    return cpuinfo.cpus;
}

int
host_get_cpu_number_of_cores(void)
{
    host_get_cpu_details();
    return cpuinfo.cores;
}

void
host_get_load_averages(sigar_loadavg_t *avg)
{
    init();
    sigar_loadavg_get(host_init.sigar, avg);
}

void
host_get_processes(sigar_proc_stat_t *procs)
{
    init();
    sigar_proc_stat_get(host_init.sigar, procs);
}

uint64_t
host_get_memory(void) 
{
    sigar_mem_t mem;
    uint64_t total;
    init();

    sigar_mem_get(host_init.sigar, &mem);
    total = mem.total / 1024;
    return total;
}

uint64_t
host_get_mem_free(void)
{
    sigar_mem_t mem;
    uint64_t free;
    init();

    sigar_mem_get(host_init.sigar, &mem);
    free = mem.free / 1024;
    return free;
}

uint64_t
host_get_swap(void)
{
    sigar_swap_t swap;
    uint64_t total;
    init();

    sigar_swap_get(host_init.sigar, &swap);
    total = swap.total / 1024;
    return total;
}

uint64_t
host_get_swap_free(void)
{
    sigar_swap_t swap;
    uint64_t free;
    init();

    sigar_swap_get(host_init.sigar, &swap);
    free = swap.free / 1024;
    return free;
}

void
host_get_cpu_details(void)
{
    int lpc = 0;
    sigar_cpu_info_list_t cpus;
    
    if(cpuinfo.cpus) {
	return;
    }

    init();
    sigar_cpu_info_list_get(host_init.sigar, &cpus);

    cpuinfo.cpus = cpus.number;
    for(lpc = 0; lpc < cpus.number; lpc++) {
	sigar_cpu_info_t *cpu = (sigar_cpu_info_t *) cpus.data;
	if(cpuinfo.model == NULL) {
	    cpuinfo.model = g_strdup(cpu->model);
	}
	cpuinfo.cores += cpu->total_cores;
    }
    
    sigar_cpu_info_list_destroy(host_init.sigar, &cpus);
}
