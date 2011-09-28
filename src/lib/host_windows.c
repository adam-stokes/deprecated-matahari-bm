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
#include <pcre.h>

#include "matahari/logging.h"
#include "matahari/host.h"
#include "host_private.h"

static const char CUSTOM_UUID_KEY[] = "CustomUUID";

static const char REBOOT_UUID_KEY[] = "RebootUUID";

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

    return NULL;
}

#if 0
/* XXX
 * This was written assuming GetTickCount64() would be available ...
 * but it's not available in mingw32.
 */

/**
 * \internal
 * \brief Get time of last boot.
 *
 * The format of the output isn't terribly important.  It just needs to
 * change each time the system is booted.
 */
static int
get_last_boot(char *last_boot, size_t len)
{
    ULONGLONG tick_count;
    SYSTEMTIME sys_time;
    FILETIME f_sys_time;
    ULARGE_INTEGER convert_again_omg;

    tick_count = GetTickCount64();

    GetSystemTime(&sys_time);
    SystemTimeToFileTime(&sys_time, &f_sys_time);
    convert_again_omg.LowPart = f_sys_time.dwLowDateTime;
    convert_again_omg.HighPart = f_sys_time.dwHighDateTime;

    /*
     * Current time minus time since boot
     *
     * - Current time is 1/10 microseconds
     * - time since boot is milliseconds
     */

    snprintf(last_boot, len, "%lu", convert_again_omg.QuadPart - (tick_count * 10000));

    mh_trace("last boot: '%s'\n", last_boot);

    return mh_strlen_zero(last_boot) ? -1 : 0;
}
#endif

char *
host_os_reboot_uuid(void)
{
#if 0
    HKEY key;
    char uuid_str[256] = "";
    DWORD uuid_str_len = sizeof(uuid_str) - 1;
    long res;
    int reset_uuid = 0;
    char last_boot[128] = "";

    if (get_last_boot(last_boot, sizeof(last_boot))) {
        mh_warn("Failed to determine time of last boot.");
        return NULL;
    }

    res = RegOpenKey(HKEY_LOCAL_MACHINE,
                     L"SYSTEM\\CurrentControlSet\\services\\Matahari",
                     &key);

    if (res != ERROR_SUCCESS) {
        mh_debug("Could not open Matahari key from the registry: %ld",
                 res);
        return NULL;
    }

    res = RegQueryValueExA(key, REBOOT_UUID_KEY, NULL, NULL,
                           (BYTE *) uuid_str, &uuid_str_len);

    if (res == ERROR_SUCCESS) {
        char *uuid_time;

        /*
         * See if a reboot has occurred since this UUID was generated.
         */

        uuid_time = strchr(uuid_str, ' ');
        if (!uuid_time) {
            mh_warn("Unexpected reboot UUID format: '%s'\n", uuid_str);
        } else {
            *uuid_time++ = '\0';
            reset_uuid = strcmp(uuid_time, last_boot);
        }
    } else {
        /*
         * No reboot UUID found at all, so generate one.
         */

        reset_uuid = 1;
    }

    if (reset_uuid) {
        UUID uuid;
        char new_uuid_str[256] = "";
        unsigned char *rs;

        UuidCreate(&uuid);

        if (UuidToStringA(&uuid, &rs) == RPC_S_OK) {
            mh_string_copy(uuid_str, (char *) rs, sizeof(uuid_str));
            snprintf(new_uuid_str, sizeof(new_uuid_str), "%s %s", uuid_str, last_boot);

            RpcStringFreeA(&rs);

            res = RegSetValueExA(key, REBOOT_UUID_KEY, 0, REG_SZ,
                                 (CONST BYTE *) uuid_str, strlen(uuid_str) + 1);

            if (res != ERROR_SUCCESS) {
                mh_warn("Failed to set reboot UUID.");
                *uuid_str = '\0';
            }
        }
    }

    RegCloseKey(key);

    return mh_strlen_zero(uuid_str) ? NULL : strdup(uuid_str);
#endif
    return NULL;
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
