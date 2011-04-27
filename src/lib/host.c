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

typedef struct host_init_s {
    sigar_t *sigar;
    gboolean sigar_init;
} host_init_t;

static host_init_t host_init = {
    .sigar      = NULL,
    .sigar_init = FALSE,
};

typedef struct cpuinfo_s {
    char *model;
    unsigned int cpus;
    unsigned int cores;
} cpuinfo_t;

static cpuinfo_t cpuinfo = {
    .model = NULL,
    .cpus  = 0,
    .cores = 0,
};

static void host_get_cpu_details(void);

static void init(void)
{
    if(host_init.sigar_init) {
        return;
    }
    sigar_open(&host_init.sigar);
    host_init.sigar_init = TRUE;
}

const char *mh_host_get_uuid(void)
{
    return mh_uuid();
}

const char *mh_host_get_hostname(void)
{
    return mh_hostname();
}

const char *mh_host_get_operating_system(void)
{
    static const char *operating_system = NULL;

    init();

    if(operating_system == NULL) {
        sigar_sys_info_t sysinfo;

        sigar_sys_info_get(host_init.sigar, &sysinfo);
        operating_system = g_strdup_printf("%s (%s)", sysinfo.vendor_name, sysinfo.version);
    }

    return operating_system;
}

int mh_host_get_cpu_wordsize(void)
{
    return (int)(CHAR_BIT * sizeof(size_t));
}

const char *mh_host_get_architecture(void)
{
    static const char *arch = NULL;

    init();

    if(arch == NULL) {
        sigar_sys_info_t sysinfo;

        sigar_sys_info_get(host_init.sigar, &sysinfo);
        arch = g_strdup(sysinfo.arch);
    }

    return arch;
}

void mh_host_reboot(void)
{
    host_os_reboot();
}

void mh_host_shutdown(void)
{
    host_os_shutdown();
}

const char *mh_host_get_cpu_model(void)
{
    host_get_cpu_details();
    return cpuinfo.model;
}

const char *mh_host_get_cpu_flags(void)
{
    return host_os_get_cpu_flags();
}

int mh_host_get_cpu_count(void)
{
    host_get_cpu_details();
    return cpuinfo.cpus;
}

int mh_host_get_cpu_number_of_cores(void)
{
    host_get_cpu_details();
    return cpuinfo.cores;
}

void mh_host_get_load_averages(sigar_loadavg_t *avg)
{
    init();
    sigar_loadavg_get(host_init.sigar, avg);
}

void mh_host_get_processes(sigar_proc_stat_t *procs)
{
    init();
    sigar_proc_stat_get(host_init.sigar, procs);
}

uint64_t mh_host_get_memory(void)
{
    sigar_mem_t mem;
    uint64_t total;
    init();

    sigar_mem_get(host_init.sigar, &mem);
    total = mem.total / 1024;
    return total;
}

uint64_t mh_host_get_mem_free(void)
{
    sigar_mem_t mem;
    uint64_t free;
    init();

    sigar_mem_get(host_init.sigar, &mem);
    free = mem.free / 1024;
    return free;
}

uint64_t mh_host_get_swap(void)
{
    sigar_swap_t swap;
    uint64_t total;
    init();

    sigar_swap_get(host_init.sigar, &swap);
    total = swap.total / 1024;
    return total;
}

uint64_t mh_host_get_swap_free(void)
{
    sigar_swap_t swap;
    uint64_t free;
    init();

    sigar_swap_get(host_init.sigar, &swap);
    free = swap.free / 1024;
    return free;
}

static void host_get_cpu_details(void)
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
