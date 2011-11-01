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

#define TIMEOUT 10
#define TA_PATH          "/usr/bin/"
#define TA_SETPROFILE    "profile"
#define TA_GETPROFILE    "active"
#define TA_LISTPROFILES  "list"
#define TA_OFF           "off"
#define TUNEDADM TA_PATH "tuned-adm"

#define SYSTEM32 "system32"
#define POWERCFG "powercfg"
#define PC_SETPROFILE "-SETACTIVE"
#define PC_GETPROFILE "-GETACTIVESCHEME"
#define PC_LISTPROFILES "-LIST"
#define GUID "GUID: "

#define STR_UNK   "UNKNOWN"
#define STR_ERR   "ERROR"


const char *
host_os_get_cpu_flags(void);

void
host_os_reboot(void);

void
host_os_shutdown(void);

/**
 * Platform specific implementation of a system beep.
 *
 * \retval 0 success
 * \retval non-zero failure
 */
int
host_os_identify(void);

char *
host_os_machine_uuid(void);

char *
host_os_ec2_instance_id(void);

char *
host_os_custom_uuid(void);

char *
host_os_reboot_uuid(void);

char *
host_os_agent_uuid(void);

int
host_os_set_custom_uuid(const char *uuid);

enum mh_result
host_os_set_power_profile(const char *profile);

enum mh_result
host_os_get_power_profile(char **profile);

GList *
host_os_list_power_profiles(void);

#endif /* __MH_HOST_PRIVATE_H__ */
