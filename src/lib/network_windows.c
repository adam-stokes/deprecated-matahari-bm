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

#include <config.h>

#include <spawn.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windns.h>
#include <iphlpapi.h>
#include <process.h>

#define GAA_FLAGS ( GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_ANYCAST )
#define BUFSIZE 1024

void
network_os_stop(const char *iface)
{
    char *exe_path;
    char *p;
    gint r;

    p = getenv("WINDIR");
    exe_path = g_strdup_printf("%s\\system32\\netsh", p);

    r = _spawnl(_P_WAIT, exe_path, exe_path, "interface",
                "set", "interface", iface, "disabled", NULL);
    g_free(exe_path);
}

void
network_os_start(const char *iface)
{
    char *exe_path;
    char *p;
    gint r;

    p = getenv("WINDIR");
    exe_path = g_strdup_printf("%s\\system32\\netsh", p);

    r = _spawnl(_P_WAIT, exe_path, exe_path, "interface",
                "set", "interface", iface, "enabled", NULL);
    g_free(exe_path);
}
