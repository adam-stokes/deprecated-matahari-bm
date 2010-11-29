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

// TODO remove this wrapper once rhbz#583747 is fixed
#include <libudev.h>

#endif

#ifdef HAVE_LIBVIRT1
#include <libvirt/libvirt.h>
#endif

#include <limits.h>
#include "matahari/host.h"
#include "host_private.h"

cpuinfo_t cpuinfo = { 0, 0, 0, 0, 0 };

const char *
host_get_uuid(void)
{
    return host_os_get_uuid();
}

const char *
host_get_hostname(void)
{
    return host_os_get_hostname();
}

const char *
host_get_operating_system(void)
{
    return host_os_get_operating_system();
}

const char *
host_get_hypervisor(void)
{
    return host_os_get_hypervisor();
}

unsigned int
host_get_platform(void)
{
  static unsigned int wordsize = 0;

  if(wordsize == 0) {
      wordsize = sizeof(void *) * CHAR_BIT;
  }

  return wordsize;
}

const char *
host_get_architecture(void)
{
    return host_os_get_architecture();
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


unsigned int
host_get_cpu_wordsize()
{
  static unsigned int wordsize = 0;

  if(wordsize == 0) {
      host_os_get_cpu_details();
      return cpuinfo.wordsize;
  }

  return wordsize;
}

const char*
host_get_cpu_model()
{
  host_os_get_cpu_details();
  return cpuinfo.model;
}

unsigned int
host_get_cpu_count()
{
  host_os_get_cpu_details();
  return cpuinfo.cpus;
}

unsigned int
host_get_cpu_number_of_cores()
{
  host_os_get_cpu_details();
  return cpuinfo.cores;
}

void
host_get_load_averages(double *one, double *five, double *fifteen)
{
    host_os_get_load_averages(one, five, fifteen);
}

unsigned int
host_get_memory(void) 
{
    return host_os_get_memory();
}

