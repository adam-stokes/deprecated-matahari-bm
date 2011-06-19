/* host_windows.c - Copyright (C) 2010 Red Hat, Inc.
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

#include <winsock.h>
#include <windows.h>
#include <winbase.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <reason.h>

#include "matahari/logging.h"
#include "matahari/host.h"
#include "host_private.h"

const char *
host_os_get_cpu_flags(void)
{
    static char *flags = NULL;

    if (!flags) {
        flags = strdup("unknown");
    }

    return flags;
}

static void
get_token_priv(HANDLE token, TOKEN_PRIVILEGES tkp)
{
    OpenProcessToken(GetCurrentProcess(),
                     TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token);
    LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME,
                         &tkp.Privileges[0].Luid);

    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    AdjustTokenPrivileges(token, FALSE, &tkp, 0,
                          (PTOKEN_PRIVILEGES) NULL, 0);
}

void
host_os_reboot(void)
{
    HANDLE token;
    TOKEN_PRIVILEGES tkp;

    get_token_priv(token, tkp);
    ExitWindowsEx(EWX_REBOOT | EWX_FORCE,
                  SHTDN_REASON_FLAG_PLANNED);
}

void
host_os_shutdown(void)
{
    HANDLE token;
    TOKEN_PRIVILEGES tkp;

    get_token_priv(token, tkp);
    ExitWindowsEx(EWX_SHUTDOWN | EWX_FORCE,
                  SHTDN_REASON_FLAG_PLANNED);
}

int
host_os_identify(void)
{
    static const long DURATION = 1000; /* 1 second */
    static const long FREQ = 440; /* 440 Hz */

    /*
     * Reference for Windows Beep():
     *     http://msdn.microsoft.com/en-us/library/ms679277%28v=vs.85%29.aspx
     */

    return Beep(FREQ, DURATION) ? 0 : -1;
}
