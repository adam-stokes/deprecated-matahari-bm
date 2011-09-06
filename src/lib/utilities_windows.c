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
    unsigned char *rs;
    long lSuccess = RegOpenKey(HKEY_LOCAL_MACHINE,
                               L"SYSTEM\\CurrentControlSet\\services\\Matahari",
                               &hKey);

    if (lSuccess != ERROR_SUCCESS) {
        mh_debug("Could not open Matahari key from the registry: %ld",
                 lSuccess);
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
        wchar_t *s_ws = NULL;
        strncpy(s, (char *)rs, MAXUUIDLEN);
        s[sizeof(s) - 1] = '\0';
        RpcStringFreeA(&rs);
        mh_trace("Got uuid: %s", s);
        s_ws = char2wide(s);
        lSuccess = RegSetValueEx(hKey, szValue, 0, REG_SZ, (CONST BYTE *)s_ws,
                                 wcslen(s_ws) * sizeof(wchar_t *));
        free(s_ws);
        free(szValue);
        if (lSuccess != ERROR_SUCCESS) {
            goto bail;
        }
    }
    return s;
 bail:
    free(szValue);
    return "";
}
