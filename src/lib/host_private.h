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

/**
 * \file
 * \brief Platform specific function prototypes.
 *
 * The functions in this header must be implemented by the platform
 * specific host data code.
 *  - host_linux.c
 *  - host_windows.c
 */

#ifndef __MH_HOST_PRIVATE_H__
#define __MH_HOST_PRIVATE_H__

#include <glib.h>

extern const char *
host_os_get_cpu_flags(void);

extern void
host_os_reboot(void);

extern void
host_os_shutdown(void);

/**
 * Platform specific implementation of a system beep.
 *
 * \retval 0 success
 * \retval non-zero failure
 */
extern int
host_os_identify(void);

extern char *host_os_machine_uuid(void);
extern char *host_os_custom_uuid(void);
extern char *host_os_reboot_uuid(void);
extern const char *host_os_agent_uuid(void);

extern int host_os_set_custom_uuid(const char *uuid);

#endif /* __MH_HOST_PRIVATE_H__ */
