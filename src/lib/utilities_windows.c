/* utilities_linux.c - Copyright (C) 2011, Red Hat, Inc.
 * Written by Russell Bryant <rbryant@redhat.com>
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

#include <windows.h>
#include <rpc.h>
#include <wchar.h>
#include "matahari/utilities.h"
#include "matahari/logging.h"
#include "utilities_private.h"

#define MAXUUIDLEN 255
#define UUID_REGISTRY_KEY "UUID"

const char *
mh_os_dnsdomainname(void)
{
    /*
     * Just use the standard domain name that sigar provides.
     *
     * XXX Is there a separate dnsdomainname for Windows?
     */

    return mh_domainname();
}

const char *
mh_os_uuid(void)
{
    UUID uuid;
    HKEY key;
    DWORD uuid_str_len = MAXUUIDLEN - 1;
    // Note: not thread safe, but neither is a ton of other code ...
    static char uuid_str[MAXUUIDLEN];
    unsigned char *rs;
    long res = RegOpenKey(HKEY_LOCAL_MACHINE,
                          L"SYSTEM\\CurrentControlSet\\services\\Matahari",
                          &key);

    if (res != ERROR_SUCCESS) {
        mh_debug("Could not open Matahari key from the registry: %ld",
                 res);
        goto bail;
    }

    res = RegQueryValueExA(key, UUID_REGISTRY_KEY, NULL, NULL,
                           (BYTE *) uuid_str, &uuid_str_len);

    if (res == ERROR_SUCCESS) {
        uuid_str[uuid_str_len] = '\0';
        return uuid_str; /* <("<) stokachu! */
    }

    UuidCreate(&uuid);

    if (UuidToStringA(&uuid, &rs) == RPC_S_OK) {
        strncpy(uuid_str, (char *) rs, sizeof(uuid_str));
        uuid_str[sizeof(uuid_str) - 1] = '\0';
        RpcStringFreeA(&rs);
        mh_trace("Got uuid: %s", uuid_str);
        res = RegSetValueExA(key, UUID_REGISTRY_KEY, 0, REG_SZ,
                             (CONST BYTE *) uuid_str, strlen(uuid_str) + 1);

        if (res != ERROR_SUCCESS) {
            goto bail;
        }
    }

    return uuid_str;

 bail:
    mh_warn("Failed to get UUID.");

    return "";
}
