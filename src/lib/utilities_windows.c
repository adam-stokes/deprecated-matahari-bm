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

#include "matahari/utilities.h"
#include "matahari/logging.h"
#include "utilities_private.h"

#define MAXUUIDLEN 1024

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
    UUID u;
    HKEY hKey;
    DWORD nSize = MAXUUIDLEN;
    wchar_t szData[MAXUUIDLEN];
    static char s[MAXUUIDLEN];
    wchar_t *szValue = char2wide("UUID");
    unsigned char __RPC_FAR *rs;
    long lSuccess = RegOpenKey(HKEY_LOCAL_MACHINE,
                               L"SYSTEM\\CurrentControlSet\\services\\Matahari",
                               &hKey);

    if (lSuccess != ERROR_SUCCESS) {
        mh_debug("Could not open Matahari key from the registry: %ld",
                 lSuccess);
        fprintf(stderr, "bail 1\n");
        goto bail;
    }

    lSuccess = RegQueryValueEx(hKey, szValue,  NULL, NULL, (LPBYTE) szData,
                               &nSize);
    if (lSuccess == ERROR_SUCCESS) {
         wcstombs(s, szData, (size_t) MAXUUIDLEN);
         return s;
    }

    UuidCreate(&u);

    if (UuidToStringA(&u, &rs) == RPC_S_OK) {
        strncpy(s,(char __RPC_FAR *)rs, MAXUUIDLEN);
        s[MAXUUIDLEN - 1] = '\0';
        RpcStringFreeA(&rs);
        mh_trace("Got uuid: %s", s);
        fprintf(stderr, "Got uuid: %s", s);
        lSuccess = RegSetValueEx(hKey, szValue, 0, REG_SZ, (CONST BYTE *)char2wide(s), strlen(s));
        if (lSuccess != ERROR_SUCCESS) {
        fprintf(stderr, "bail 3\n");
            goto bail;
        }
    }
    return s;
 bail:
    fprintf(stderr, "BAiLED\n");
    return "";
}
