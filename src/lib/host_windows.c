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
#include <wininet.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <reason.h>
#include <pcre.h>

#include "matahari/logging.h"
#include "matahari/host.h"
#include "matahari/errors.h"
#include "host_private.h"

static const char CUSTOM_UUID_KEY[] = "CustomUUID";

static const char REBOOT_UUID_KEY[] = "RebootUUID";

#define BUFSIZE 4096

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
    gchar *output = NULL, *begin, *end;
    char *ret;
    GError *error = NULL;
    gboolean res;
    gchar *argv[] = { "WMIC", "CSPRODUCT", "Get", "UUID", NULL };

    res = g_spawn_sync(NULL, argv, NULL, G_SPAWN_SEARCH_PATH,
                NULL, NULL, &output, NULL, NULL, &error);

    if (res == FALSE) {
        mh_err("Failed to run WMIC.exe to get SMBIOS UUID: %s\n", error->message);
        g_error_free(error);
        error = NULL;
    }

    if (!output) {
        mh_err("Got no output from WMIC.exe when trying to get SMBIOS UUID.\n");
        return NULL;
    }

    if (!(begin = strstr(output, "\r\n"))) {
        mh_err("Unexpected format of output: '%s'", output);
        g_free(output);
        return NULL;
    }

    begin += 2;

    if (mh_strlen_zero(begin)) {
        mh_err("Unexpected format of output: '%s'", output);
        g_free(output);
        return NULL;
    }

    if (!(end = strstr(begin, "\r\n"))) {
        mh_err("Unexpected format of output: '%s'", output);
        g_free(output);
        return NULL;
    }

    *end = '\0';

    ret = strdup(begin);

    g_free(output);
    output = NULL;

    return ret;
}

char *
host_os_ec2_instance_id(void)
{
    static const char URI[] = "http://169.254.169.254/latest/meta-data/instance-id";
    HINTERNET internet = NULL;
    HINTERNET open_url = NULL;
    DWORD bytes_read = 0;
    char buf[256] = "";

    internet = InternetOpenA("Matahari", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!internet) {
        mh_err("Failed to open the internets (%lu)", (unsigned long) GetLastError());
        return NULL;
    }

    open_url = InternetOpenUrlA(internet, URI, NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!open_url) {
        mh_err("Failed to open URL '%s' (%lu)", URI, (unsigned long) GetLastError());
        goto return_cleanup;
    }

    while (InternetReadFile(open_url, buf + strlen(buf),
                            sizeof(buf) - strlen(buf) - 1,
                            &bytes_read) == FALSE || bytes_read != 0)
    {
        /* Mad commenting in a while loop, what what */
    }

return_cleanup:

    if (open_url && InternetCloseHandle(open_url) == FALSE) {
        mh_err("Failed to close the HTTP request. (%lu)", (unsigned long) GetLastError());
    }

    if (internet && InternetCloseHandle(internet) == FALSE) {
        mh_err("Failed to close the internets. (%lu)", (unsigned long) GetLastError());
    }

    return mh_strlen_zero(buf) ? NULL : strdup(buf);
}

/**
 * \internal
 * \brief Get time of last boot.
 */
static int
get_last_boot(char *last_boot, size_t len)
{
    gchar *output = NULL;
    GError *error = NULL;
    gboolean res;
    gchar *argv[] = { "WMIC", "OS", "Get", "LastBootUpTime", NULL };

    if (len) {
        *last_boot = '\0';
    }

    res = g_spawn_sync(NULL, argv, NULL, G_SPAWN_SEARCH_PATH,
                NULL, NULL, &output, NULL, NULL, &error);

    if (res == FALSE) {
        mh_err("Failed to run WMIC.exe to get last boot time: %s\n", error->message);
        g_error_free(error);
        error = NULL;
    }

    if (!output) {
        mh_err("Got no output from WMIC.exe when trying to get last boot time.\n");
        return -1;
    }

    mh_string_copy(last_boot, output, len);

    g_free(output);
    output = NULL;

    return mh_strlen_zero(last_boot) ? -1 : 0;
}

char *
host_os_reboot_uuid(void)
{
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

    mh_trace("last_boot: '%s'", last_boot);

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
            reset_uuid = 1;
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
                                 (CONST BYTE *) new_uuid_str, strlen(new_uuid_str) + 1);

            if (res != ERROR_SUCCESS) {
                mh_warn("Failed to set reboot UUID.");
                *uuid_str = '\0';
            }
        }
    }

    RegCloseKey(key);

    return mh_strlen_zero(uuid_str) ? NULL : strdup(uuid_str);
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

static enum mh_result
exec_command(const char *apath, char *args[], int atimeout, char **stdoutbuf,
             char **stderrbuf)
{
    enum mh_result res = MH_RES_SUCCESS;
    DWORD status = 0;
    gint stdout_fd;
    gint stderr_fd;
    HANDLE phandle = NULL;
    GError *gerr = NULL;

    if (!args || !args[0])
        return MH_RES_OTHER_ERROR;

    /* convert to ms */
    atimeout = atimeout * 1000;

    mh_trace("Spawning '%s'\n", args[0]);
    if (!g_spawn_async_with_pipes(apath, args, NULL, G_SPAWN_DO_NOT_REAP_CHILD,
                                  NULL, NULL, (GPid *) &phandle, NULL,
                                  &stdout_fd, &stderr_fd, &gerr)) {
        mh_perror(LOG_ERR, "Spawn_process failed with code %d, message: %s\n",
                  gerr->code, gerr->message);
        return MH_RES_OTHER_ERROR;
    }

    mh_trace("Waiting for %p", (void *)phandle);
    WaitForSingleObject(phandle, atimeout);

    if (GetExitCodeProcess(phandle, &status) == 0) {
        mh_err("Could not get exit code: %lu", GetLastError());
        res = MH_RES_BACKEND_ERROR;
    }

    if (res == MH_RES_BACKEND_ERROR) {
        TerminateProcess(phandle, 1);
        mh_err("%p - timed out after %dms", (void *)phandle, atimeout);
    }

    /* Nice to log it somewhere */
    mh_debug("Result of '%s' was %d", args[0], (int)status);

    mh_trace("Child done: %p", (void *)phandle);

    if (stdoutbuf) {
        if (mh_read_from_fd(stdout_fd, stdoutbuf) < 0) {
            mh_err("Unable to read standard output from command %s.", apath);
            stdoutbuf = NULL;
        } else {
            mh_debug("stdout: %s", *stdoutbuf);
        }
    }

    if (stderrbuf) {
        if (mh_read_from_fd(stderr_fd, stderrbuf) < 0) {
            mh_err("Unable to read standard error output from command %s.", apath);
            stderrbuf = NULL;
        } else {
            mh_debug("stderr: %s", *stderrbuf);
        }
    }

    _close(stdout_fd);
    _close(stderr_fd);

    g_spawn_close_pid((GPid *) phandle);

    return res;
}

static gboolean
get_profiles(GList **names, GList **guids)
{
    gboolean rc = FALSE;
    int len;
    char *stdoutbuf = NULL;
    char *c1, *c2;
    char *args[3] = {0};

    args[0] = SYSTEM32 "\\" POWERCFG;
    args[1] = PC_LISTPROFILES;
    args[2] = NULL;

    if (exec_command(getenv("WINDIR"), args , TIMEOUT, &stdoutbuf, NULL)
            == MH_RES_SUCCESS && stdoutbuf) {
        rc = TRUE;
        len = strlen(stdoutbuf);
        c1 = stdoutbuf;

        // powercfg command returns lines with profiles, that looks like:
        // "Power Scheme GUID: 381b4222-f694-41f0-9685-ff5bb260df2e  (Balanced)"
        // So we will take word after "GUID: " as GUID and word in brackets
        // as name of the profile
        while ((c1 = strstr(c1, GUID))) {
            c1 += strlen(GUID);
            // c1 is now after "GUID: "
            if (c1 - stdoutbuf < len && (c2 = strchr(c1, ' '))) {
                // c2 points to end of uuid, put \0 there
                *c2 = '\0';
                // Add guid to the list
                *guids = g_list_append(*guids, strdup(c1));
                // Put \n back, so the string can be freed
                *c2 = '\n';
                // Now find name in brackets
                if ((c1 = strchr(c2, '(')) && ++c1 - stdoutbuf < len &&
                    (c2 = strchr(c1, ')'))) {
                    // Append profile name to the list
                    *c2 = '\0';
                     *names = g_list_append(*names, strdup(c1));
                    *c2 = ')';
                }
            }
        }
    }

    if (!*names || !*guids || g_list_length(*names) != g_list_length(*guids))
        rc = FALSE;

    free(stdoutbuf);
    return rc;
}

enum mh_result
host_os_set_power_profile(const char *profile)
{
    enum mh_result res = MH_RES_SUCCESS;
    char *guid = NULL;
    GList *names = NULL;
    GList *guids = NULL;
    GList *pnames = NULL;
    GList *pguids = NULL;
    char *args[4] = {0};

    if (!get_profiles(&names, &guids)) {
        return MH_RES_BACKEND_ERROR;
    }

    for (pnames = g_list_first(names), pguids = g_list_first(guids);
         pnames && pguids && !guid;
         pnames = g_list_next(pnames), pguids = g_list_next(pguids)) {
        if (!strcmp((char *) pnames->data, profile)) {
            guid = strdup((char *) pguids->data);
        }
    }

    g_list_free_full(names, free);
    g_list_free_full(guids, free);

    if (guid) {
        args[0] = SYSTEM32 "\\" POWERCFG;
        args[1] = PC_SETPROFILE;
        args[2] = guid;
        args[3] = NULL;
        res = exec_command(getenv("WINDIR"), args, TIMEOUT, NULL, NULL);
        free(guid);
    } else {
        res = MH_RES_INVALID_ARGS;
    }
    return res;
}

enum mh_result
host_os_get_power_profile(char **profile)
{
    enum mh_result res;
    char *stdoutbuf = NULL;
    char *c1, *c2;
    char *args[3] = {0};

    args[0] = SYSTEM32 "\\" POWERCFG;
    args[1] = PC_GETPROFILE;
    args[2] = NULL;

    res = exec_command(getenv("WINDIR"), args, TIMEOUT, &stdoutbuf, NULL);
    if (res == MH_RES_SUCCESS) {
        // Profile name is string between brackets
        if (stdoutbuf && (c1 = strchr(stdoutbuf, '(')) &&
                ++c1 - stdoutbuf < strlen(stdoutbuf) && (c2 = strchr(c1, ')'))) {
            *c2 = '\0';
            *profile = strdup(c1);
            *c2 = ')';
        } else {
            res = MH_RES_BACKEND_ERROR;
        }
    }
    free(stdoutbuf);

    return res;
}

GList *
host_os_list_power_profiles(void)
{
    GList *names = NULL;
    GList *guids = NULL;

    get_profiles(&names, &guids);
    g_list_free_full(guids, free);

    return names;
}
