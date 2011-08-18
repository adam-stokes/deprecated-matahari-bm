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

static void
host_get_cpu_details(void);

static void
init(void)
{
    if (host_init.sigar_init) {
        return;
    }
    sigar_open(&host_init.sigar);
    host_init.sigar_init = TRUE;
}

const char *
mh_host_get_hostname(void)
{
    return mh_hostname();
}

const char *
mh_host_get_operating_system(void)
{
    static const char *operating_system = NULL;

    init();

    if (operating_system == NULL) {
        sigar_sys_info_t sys_info;

        sigar_sys_info_get(host_init.sigar, &sys_info);
        operating_system = g_strdup_printf("%s (%s)", sys_info.vendor_name,
                                           sys_info.version);
    }

    return operating_system;
}

int
mh_host_get_cpu_wordsize(void)
{
    return (int)(CHAR_BIT * sizeof(size_t));
}

const char *
mh_host_get_architecture(void)
{
    static const char *arch = NULL;

    init();

    if (arch == NULL) {
        sigar_sys_info_t sys_info;

        sigar_sys_info_get(host_init.sigar, &sys_info);
        arch = g_strdup(sys_info.arch);
    }

    return arch;
}

void
mh_host_reboot(void)
{
    host_os_reboot();
}

void
mh_host_shutdown(void)
{
    host_os_shutdown();
}

const char *
mh_host_get_cpu_model(void)
{
    host_get_cpu_details();
    return cpuinfo.model;
}

const char *
mh_host_get_cpu_flags(void)
{
    return host_os_get_cpu_flags();
}

int
mh_host_get_cpu_count(void)
{
    host_get_cpu_details();
    return cpuinfo.cpus;
}

int
mh_host_get_cpu_number_of_cores(void)
{
    host_get_cpu_details();
    return cpuinfo.cores;
}

void
mh_host_get_load_averages(sigar_loadavg_t *avg)
{
    init();
    sigar_loadavg_get(host_init.sigar, avg);
}

void
mh_host_get_processes(sigar_proc_stat_t *procs)
{
    init();
    sigar_proc_stat_get(host_init.sigar, procs);
}

uint64_t
mh_host_get_memory(void)
{
    sigar_mem_t mem;
    uint64_t total;
    init();

    sigar_mem_get(host_init.sigar, &mem);
    total = mem.total / 1024;
    return total;
}

uint64_t
mh_host_get_mem_free(void)
{
    sigar_mem_t mem;
    uint64_t free_mem;
    init();

    sigar_mem_get(host_init.sigar, &mem);
    free_mem = mem.free / 1024;
    return free_mem;
}

uint64_t
mh_host_get_swap(void)
{
    sigar_swap_t swap;
    uint64_t total;
    init();

    sigar_swap_get(host_init.sigar, &swap);
    total = swap.total / 1024;
    return total;
}

uint64_t
mh_host_get_swap_free(void)
{
    sigar_swap_t swap;
    uint64_t free_mem;
    init();

    sigar_swap_get(host_init.sigar, &swap);
    free_mem = swap.free / 1024;
    return free_mem;
}

static void
host_get_cpu_details(void)
{
    sigar_cpu_info_list_t procs;

    if (cpuinfo.cpus) {
        return;
    }

    init();
    sigar_cpu_info_list_get(host_init.sigar, &procs);

    cpuinfo.cpus = procs.number;
    if (procs.number) {
        sigar_cpu_info_t *proc = (sigar_cpu_info_t *)procs.data;
        cpuinfo.model = g_strdup(proc->model);
        cpuinfo.cores = proc->total_cores;
    }

    sigar_cpu_info_list_destroy(host_init.sigar, &procs);
}

int
mh_host_identify(void)
{
    return host_os_identify();
}

static char *custom_uuid = NULL;
const char *
mh_host_get_uuid(const char *lifetime)
{
    const char *uuid = NULL;
    static const char *immutable_uuid = NULL;
    static const char *hardware_uuid = NULL;
    static const char *reboot_uuid = NULL;
    static const char *agent_uuid = NULL;

    if (lifetime == NULL || strcasecmp("Filesystem", lifetime) == 0) {
        if (immutable_uuid == NULL) {
            immutable_uuid = mh_uuid();
        }
        uuid = immutable_uuid;

    } else if (strcasecmp("Hardware", lifetime) == 0) {
        if(hardware_uuid == NULL) {
            hardware_uuid = host_os_machine_uuid();
        }
        uuid = hardware_uuid;

    } else if (strcasecmp("Reboot", lifetime) == 0) {

        if(reboot_uuid == NULL) {
            reboot_uuid = host_os_reboot_uuid();
        }
        uuid = reboot_uuid;

    } else if (strcasecmp("Agent", lifetime) == 0) {
        if(agent_uuid == NULL) {
            agent_uuid = host_os_agent_uuid();
        }
        uuid = agent_uuid;

    } else if (strcasecmp("Custom", lifetime) == 0) {
        if(custom_uuid == NULL) {
            custom_uuid = host_os_custom_uuid();
        }
        uuid = custom_uuid;

    } else {
        uuid = "invalid-lifetime";
    }

    if (uuid == NULL) {
        uuid = "not-available";
    }
    return uuid;
}


int
mh_host_set_uuid(const char *lifetime, const char *uuid)
{
    if (lifetime && strcmp("Custom", lifetime) == 0) {
        int rc = host_os_set_custom_uuid(uuid);
        free(custom_uuid);
        custom_uuid = host_os_custom_uuid();
        return rc;
    }

    return G_FILE_ERROR_NOSYS;
}
