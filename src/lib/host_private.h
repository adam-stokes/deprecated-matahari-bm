/* host_private.h - Copyright (C) 2010 Red Hat, Inc.
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
					
#include <glib.h>

typedef struct cpuinfo_s
{
  char *model;
  unsigned int cpus;
  unsigned int cores;
} cpuinfo_t;

extern cpuinfo_t cpuinfo;

typedef struct host_init_s
{
    sigar_t *sigar;
    gboolean sigar_init;
} host_init_t;

extern host_init_t host_init;

extern const char *host_os_get_uuid(void);
extern const char *host_os_get_hostname(void);
extern const char *host_os_get_operating_system(void);
extern const char *host_os_get_architecture(void);

extern unsigned int host_os_get_memory(void);
extern unsigned int host_os_get_platform(void);

extern void host_os_reboot(void);
extern void host_os_shutdown(void);

extern void host_os_get_cpu_details(void);
