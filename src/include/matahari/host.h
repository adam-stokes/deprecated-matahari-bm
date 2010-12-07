#ifndef __HOST_H
#define __HOST_H

/* host.h - Copyright (C) 2009 Red Hat, Inc.
 * Written by Arjun Roy <arroy@redhat.com>
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

extern const char *host_get_uuid(void);
extern const char *host_get_hostname(void);
extern const char *host_get_operating_system(void);

extern const char *host_get_architecture(void);
extern const char *host_get_cpu_model(void);

extern unsigned int host_get_memory(void);
extern unsigned int host_get_cpu_count(void);
extern unsigned int host_get_cpu_number_of_cores(void);

extern unsigned int host_get_platform(void);
extern unsigned int host_get_cpu_wordsize(void);

extern void host_identify(const unsigned int iterations);

extern void host_reboot(void);
extern void host_shutdown(void);
extern void host_get_load_averages(double *one, double *five, double *fifteen);

#endif // __HOST_H
