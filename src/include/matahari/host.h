#ifndef __HOST_H
#define __HOST_H

/* host.h - Copyright (C) 2009 Red Hat, Inc.
 * Written by Arjun Roy <arroy@redhat.com>
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

extern void host_os_get_cpu_details(void);

extern const char *host_get_uuid(void);
extern const char *host_os_get_uuid(void);

extern const char *host_get_hostname(void);
extern const char *host_os_get_hostname(void);

extern const char *host_get_operating_system(void);
extern const char *host_os_get_operating_system(void);

extern const char *host_get_hypervisor(void);
extern const char *host_os_get_hypervisor(void);

extern unsigned int host_get_platform(void);
extern unsigned int host_os_get_platform(void);

extern const char *host_get_architecture(void);
extern const char *host_os_get_architecture(void);

extern unsigned int host_get_cpu_wordsize(void);

extern const char *host_get_cpu_model(void);

extern unsigned int host_get_cpu_count(void);

extern unsigned int host_get_cpu_number_of_cores(void);

extern void host_identify(const unsigned int iterations);

extern void host_shutdown(void);
extern void host_os_shutdown(void);

extern void host_reboot(void);
extern void host_os_reboot(void);

extern unsigned int host_get_memory(void);
extern unsigned int host_os_get_memory(void);

extern void host_get_load_averages(double *one, double *five, double *fifteen);
extern void host_os_get_load_averages(double *one, double *five, double *fifteen);

#endif // __HOST_H
