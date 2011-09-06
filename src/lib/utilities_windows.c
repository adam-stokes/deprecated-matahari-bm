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

#define MAXUUIDLEN 37

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
    uuid_t u;
    static char s[MAXUUIDLEN];
    unsigned char __RPC_FAR *rs;

    UuidCreate(u);

    if (UuidToStringA((UUID *)u, &rs) == RPC_S_OK) {
        strncpy(s,(char __RPC_FAR *)rs, MAXUUIDLEN);
        RpcStringFree(&rs);
        mh_trace("Got uuid: %s", s);
        return s;
    }
    return "";
}
