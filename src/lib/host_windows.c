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

#include "config.h"

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

static const char CUSTOM_UUID_KEY[] = "CustomUUID";

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
enable_se_priv(void)
{
    HANDLE token;
    TOKEN_PRIVILEGES tkp;

    LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME,
                         &tkp.Privileges[0].Luid);

    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (OpenProcessToken(GetCurrentProcess(),
                         TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token)) {
        AdjustTokenPrivileges(token, FALSE, &tkp, 0,
                              (PTOKEN_PRIVILEGES) NULL, 0);
        CloseHandle(token);
    }
}

void
host_os_reboot(void)
{
    enable_se_priv();
    ExitWindowsEx(EWX_REBOOT | EWX_FORCE,
                  SHTDN_REASON_FLAG_PLANNED);
}

void
host_os_shutdown(void)
{
    enable_se_priv();
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

char *
host_os_machine_uuid(void)
{
#if 0
    UINT res;
    char *uuid = NULL;
    struct RawSMBIOSDData *smbios_data;

    /*
     * Doc on SMBIOS support in windows:
     *     http://msdn.microsoft.com/en-us/windows/hardware/gg463136
     *
     * See GetSystemFirmwareTable()
     *     http://msdn.microsoft.com/en-us/library/ms724379%28v=VS.85%29.aspx
     */

    res = GetSystemFirmwareTable('RSMB', 0, NULL, 0);

    if (!res) {
        mh_info("Failed to get required SMBIOS buffer size.");
        return NULL;
    }

    if (!(buf = malloc(res))) {
        return NULL;
    }

    res = GetSystemFirmwareTable('RSMB', 0, (void *) smbios_data, res);

    if (!res) {
        mh_info("Failed to get SMBIOS data.");
        goto return_cleanup;
    }

    uuid = find_uuid(smbios_data->SMBIOSTableData, smbios_data->Length);

return_cleanup:

    free(buf);

    return uuid;
#endif

    /*
     * GetSystemFirmwareTable() does not appear to be available with mingw32.
     */

    return strdup("not-implemented");
}

char *
host_os_reboot_uuid(void)
{
    return strdup("not-implemented");
}

char *
host_os_agent_uuid(void)
{
    UUID uuid;
    unsigned char *rs = NULL;

    if (rs) {
        goto return_uuid;
    }

    UuidCreate(&uuid);

    if (UuidToStringA(&uuid, &rs) != RPC_S_OK) {
        mh_err("Failed to convert agent UUID to string");
    }

return_uuid:

    return strdup(rs ? (char *) rs : "");
}

char *
host_os_custom_uuid(void)
{
    HKEY key;
    char uuid_str[256] = "";
    DWORD uuid_str_len = sizeof(uuid_str) - 1;
    long res;

    res = RegOpenKey(HKEY_LOCAL_MACHINE,
                     L"SYSTEM\\CurrentControlSet\\services\\Matahari",
                     &key);

    if (res != ERROR_SUCCESS) {
        mh_debug("Could not open Matahari key from the registry: %ld",
                 res);
        return NULL;
    }

    res = RegQueryValueExA(key, CUSTOM_UUID_KEY, NULL, NULL,
                           (BYTE *) uuid_str, &uuid_str_len);

    if (res != ERROR_SUCCESS) {
        mh_warn("Failed to get custom UUID.");
    }

    RegCloseKey(key);

    return mh_strlen_zero(uuid_str) ? NULL : strdup(uuid_str);
}

int
host_os_set_custom_uuid(const char *uuid)
{
    HKEY key;
    long res;
    int ret = 0;

    res = RegOpenKey(HKEY_LOCAL_MACHINE,
                     L"SYSTEM\\CurrentControlSet\\services\\Matahari",
                     &key);

    if (res != ERROR_SUCCESS) {
        mh_debug("Could not open Matahari key from the registry: %ld",
                 res);
        return -1;
    }

    res = RegSetValueExA(key, CUSTOM_UUID_KEY, 0, REG_SZ,
                         (CONST BYTE *) uuid, strlen(uuid) + 1);

    if (res != ERROR_SUCCESS) {
        mh_err("Failed to set custom UUID: %ld\n", res);
        ret = -1;
    }

    RegCloseKey(key);

    return ret;
}
