/*
 * Copyright (C) 2010 Andrew Beekhof <andrew@beekhof.net>
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
#include <windows.h>
#include <tchar.h>
#include <stdio.h>

#include "matahari/logging.h"
#include "matahari/mainloop.h"
#include "matahari/services.h"
#include "matahari/utilities.h"
#include "services_private.h"

/* Some code adapted from http://msdn.microsoft.com/en-us/library/ms682499(v=vs.85).aspx */

static void
create_service_key(const wchar_t *template, wchar_t *buffer, int size,
                   char *name)
{
#ifdef HAVE_SWPRINTF_S
    swprintf_s
#else
    snwprintf
#endif
        (buffer, size, template, name);
}

static gboolean
windows_service_control(char *name, int command)
{
    HKEY     key_service;
    BOOL     fSuccess = TRUE;
    char     raw[100];
    size_t   value_bytes = 0;
    wchar_t *value = NULL;
    wchar_t  szRegistryPath[1024];

    create_service_key(L"SYSTEM\\CurrentControlSet\\services\\%s",
                       szRegistryPath, DIMOF(szRegistryPath), name);

    fSuccess = RegOpenKey(HKEY_LOCAL_MACHINE, szRegistryPath, &key_service);
    if (fSuccess != ERROR_SUCCESS) {
        printf("Could not read key at '%ls' - %d\n\r", szRegistryPath,
               fSuccess);
        return FALSE;
    }

    itoa(command, raw, 10);
    value = char2wide(raw);
    value_bytes = 2 * (lstrlen(value) + 1);

    fSuccess = RegSetValueEx(key_service, L"Start", 0, REG_SZ, (LPBYTE) value,
                             value_bytes);
    mh_info("Setting Start='%ls' for %s: %s (%d)\n\r", value, name,
            fSuccess == ERROR_SUCCESS ? "PASS" : "FAIL", fSuccess);

    RegCloseKey(key_service);
    free(value);

    return fSuccess == ERROR_SUCCESS;
}

#define BUFSIZE 4096
static void
read_output(HANDLE h, char **output, int *length)
{
    /* Read output from the child process's pipe for STDOUT
     * Stop when there is no more data.
     */
    int len = 0;
    DWORD bytes = 0;
    char buf[BUFSIZE];
    char *data = NULL;

    while (1) {
        if (ReadFile(h, buf, BUFSIZE, &bytes, NULL) == 0) {
            break;
        } else if (bytes == 0) {
            break;
        }

        data = realloc(data, len + (int) bytes + 1);
        mh_info("Read %d: %.*s", len, (int) bytes, buf);
        sprintf(data + len, "%s", buf);
        data[len + bytes] = 0;
        len += (int)bytes;
    }

    *length = len;
    *output = data;
    mh_info("Read %d bytes", len);
}

static int
extract_service_status(const char *unmutable, int max)
{
    int lpc = 0, last = 0;

    if (unmutable == NULL || max < 10) {
        return LSB_STATUS_OTHER_ERROR;
    }

    for (; lpc < max; lpc++) {
        switch (unmutable[lpc]) {
        case '\n':
        case '\r':
        case 0:
            if (lpc - last > 1) {
                char name[512];
                char value[512];
                char next[2048];
                /* char *next = malloc(1+lpc-last); */
                int rc = 0, number = 0;

                strncpy(next, unmutable + last, lpc - last);
                next[lpc - last] = 0;

                rc = sscanf(next, "        %[^:]: %d %[^:]", name, &number,
                            value);
                if (rc == 3) {
                    mh_info("  '%s' = '%s' %d", name, value, number);
                    if (strncmp("STATE", name, 5) == 0) {
                        switch (number) {
                        case 1:
                            rc = LSB_STATUS_NOT_RUNNING;
                            break;
                        case 3:
                            /* Stopping but not stopped - treat as active */
                            rc = LSB_STATUS_OK;
                            break;
                        case 4:
                            rc = LSB_STATUS_OK;
                            break;
                        default:
                            rc = LSB_STATUS_OTHER_ERROR;
                            mh_info("Unknown status: %s", next);
                        }
                        /* free(next); */
                        return rc;
                    }
                }
                /* free(next); */
            }
            last = lpc + 1;
            break;
        }
    }
    mh_info("Couldn't parse service status");
    return LSB_STATUS_OTHER_ERROR;
}

gboolean
services_os_action_execute(svc_action_t *op, gboolean synchronous)
{
    int max = 0;
    DWORD status = 0;
    wchar_t *exec_w = NULL;
    STARTUPINFO siStartInfo;
    SECURITY_ATTRIBUTES saAttr;
    PROCESS_INFORMATION piProcInfo;

    HANDLE child_pipe_rd = NULL;
    HANDLE child_pipe_wr = NULL;

    gboolean is_status_op = FALSE;

    if (strcmp("status", op->action) == 0) {
        is_status_op = TRUE;

    } else if (strcmp("disable", op->action) == 0) {
        return windows_service_control(op->rsc, SERVICE_DISABLED);

    } else if (strcmp("enable", op->action) == 0) {
        return windows_service_control(op->rsc, SERVICE_AUTO_START);
    }

    /* Initialize all structures */
    ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
    ZeroMemory(&saAttr, sizeof(SECURITY_ATTRIBUTES));
    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

    /* Set the bInheritHandle flag so pipe handles are inherited.  */
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    /* Create a pipe for the child process's stdout  */
    if (!CreatePipe(&child_pipe_rd, &child_pipe_wr, &saAttr, 0)) {
        mh_err("Couldn't create child pipe");
        goto fail;
    }

    /* Ensure the read handle to the pipe for STDOUT is not inherited. */
    if (!SetHandleInformation(child_pipe_rd, HANDLE_FLAG_INHERIT, 0)) {
        mh_err("Couldn't set pipe handle information");
        goto fail;
    }

    siStartInfo.cb = sizeof(STARTUPINFO);
    siStartInfo.hStdError = child_pipe_wr;
    siStartInfo.hStdOutput = child_pipe_wr;
/*   siStartInfo.hStdInput = g_hChildStd_IN_Rd; */
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    exec_w = char2wide(op->opaque->exec);
    if (!CreateProcess(NULL,
                       exec_w,         // command line
                       NULL,           // process security attributes
                       NULL,           // primary thread security attributes
                       TRUE,           // handles are inherited
                       0,              // creation flags
                       NULL,           // use parent's environment
                       NULL,           // use parent's current directory
                       &siStartInfo,   // STARTUPINFO pointer
                       &piProcInfo)) { // receives PROCESS_INFORMATION
        mh_err("Could not create child process");
        goto fail;

        /* For now, run all commands synchronously */
        /* } else if(synchronous == FALSE) { */
        /* 	return TRUE; */
    }

    WaitForSingleObject(piProcInfo.hProcess, op->timeout /* Or INFINITE */);

    if (GetExitCodeProcess(piProcInfo.hProcess, &status) == 0) {
        mh_err("Could not get exit code: %lu", GetLastError());
        status = STILL_ACTIVE;
    }

    if (status == STILL_ACTIVE) {
        op->rc = LSB_OTHER_ERROR;
        op->status = LRM_OP_TIMEOUT;
        TerminateProcess(piProcInfo.hProcess, 1);
        mh_err("Operation for %s timeout out after %dms", op->id, op->timeout);

    } else if (status == 1062 && strcmp("stop", op->action) == 0) {
        /* Stopping something thats stopped is a success */
        op->status = LRM_OP_DONE;
        op->rc = LSB_OK;

    } else if (status == 1056 && strcmp("start", op->action) == 0) {
        /* Starting something thats started is a success */
        op->status = LRM_OP_DONE;
        op->rc = LSB_OK;

    } else if (status == 0) {
        op->status = LRM_OP_DONE;
        op->rc = LSB_OK;

    } else {
        op->status = LRM_OP_ERROR;
        op->rc = LSB_OTHER_ERROR;
        mh_info("Result of '%ls' for '%s' was %d", exec_w, op->id, (int)status);
    }

    /* Nice to log it somewhere */
    if (op->rc == LSB_OK) {
        mh_debug("Result of '%ls' for '%s' was %d", exec_w, op->id, (int)status);
    }

    /* Close the write end of the pipe before reading from the
     * read end of the pipe, to control child process execution.
     * The pipe is assumed to have enough buffer space to hold the
     * data the child process has already written to it.
     */
    if (!CloseHandle(child_pipe_wr)) {
        mh_err("Couldn't close write end of child pipe");
    }

    read_output(child_pipe_rd, &op->stdout_data, &max);
    if (op->stdout_data) {
        mh_debug("RAW: %s", op->stdout_data);
    }

    if (is_status_op && op->status == LRM_OP_DONE) {
        op->rc = extract_service_status(op->stdout_data, max);
    }

    CloseHandle(piProcInfo.hProcess);
    CloseHandle(piProcInfo.hThread);
    CloseHandle(child_pipe_rd);

    if (is_status_op && op->rc == LSB_OTHER_ERROR) {
        op->rc = LSB_STATUS_OTHER_ERROR;
    }

    return TRUE;

fail:
    op->status = LRM_OP_ERROR;
    op->rc = LSB_OTHER_ERROR;
    if (is_status_op && op->rc == LSB_OTHER_ERROR) {
        op->rc = LSB_STATUS_OTHER_ERROR;
    }
    return FALSE;
}

void
services_os_set_exec(svc_action_t *op)
{
    char *p = getenv("WINDIR");
    if (strcmp("status", op->action) == 0) {
        op->opaque->exec = g_strdup_printf("%s\\system32\\sc.exe query %s", p,
                                           op->rsc);
        /* op->opaque->exec = g_strdup_printf("sc query %s", op->rsc); */

    } else {
        op->opaque->exec = g_strdup_printf("%s\\system32\\sc.exe %s %s", p,
                                           op->action, op->rsc);
        /* op->opaque->exec = g_strdup_printf("sc %s %s", op->action, op->rsc); */
    }
}

GList *
services_os_get_directory_list(const char *root, gboolean files)
{
    /* For now, this is unsupported/unnecessary on Windows,
     * return an empty list */
    return NULL;
}

GList *
services_os_list(void)
{
    /* Need to implement */
    /* SC QUERY state= all | findstr "SERVICE_NAME"     */
    return NULL;
}

GList *
resources_os_list_ocf_providers(void)
{
    /* Unsupported on Windows, return an empty list */
    return NULL;
}

GList *
resources_os_list_ocf_agents(const char *provider)
{
    /* Unsupported on Windows, return an empty list */
    return NULL;
}
