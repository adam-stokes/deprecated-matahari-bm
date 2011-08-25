/*
 * network_windows.c: windows network functions
 *
 * Copyright (C) 2010 Red Hat Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 *
 * Author: Adam Stokes <astokes@fedoraproject.org>
 */

#ifndef WINVER
# define WINVER 0x0501
#endif

#include "matahari/network.h"
#include "matahari/utilities.h"

#include <config.h>

#include <glib.h>
#include <windows.h>

static void
network_os_setstate(const char *iface, const char *state)
{
    PROCESS_INFORMATION pi;
    STARTUPINFO si;
    char *p;
    char *exe_path;
    wchar_t *wexe_path;

    p = getenv("WINDIR");
    exe_path = g_strdup_printf("\"%s\\system32\\netsh\" "
                               "interface set interface %s %s",
                               p, iface, state);
    wexe_path = char2wide(exe_path);
    g_free(exe_path);

    CreateProcess(NULL, wexe_path, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    free(wexe_path);
}

void
network_os_stop(const char *iface)
{
    network_os_setstate(iface, "disabled");
}

void
network_os_start(const char *iface)
{
    network_os_setstate(iface, "enabled");
}
