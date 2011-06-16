/* t_dnssrv.c - Copyright (c) 2011 Red Hat, Inc.
 * Written by Adam Stokes <astokes@fedoraproject.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <sys/utsname.h> 
#include <netdb.h>      
#include <string.h>     
#include <stdio.h>      
#include <glib.h>
#include "dnssrv_private.h"


int 
gethostdomain(char* domain, size_t domainlen)
{
    struct utsname sysinfo;
    struct addrinfo hints, *res, *p;
    int error;
    char* offset;

    if (uname(&sysinfo) != 0) {
        return -1;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_CANONNAME;

    if ((error = getaddrinfo(sysinfo.nodename, NULL, &hints, &res)) != 0) {
        printf("warning: %s", gai_strerror(error));
        goto fail;
    }

    /* strip host from fqdn */
    if ((offset = strchr(res->ai_canonname, '.')) == NULL) {
        goto fail;
    }
    strncpy(domain, ++offset, domainlen);
    freeaddrinfo(res);

    return 0;
    
fail:
    freeaddrinfo(res);    
    return -1;
}


int main(int argc, char **argv)
{
    struct srv_reply **srvl = NULL, *srv = NULL;
    int i;

    char hostdomain[MAX_NAME_LEN];
    if (gethostdomain(hostdomain, sizeof hostdomain) != 0) {
        perror("Could not determine host domain");
        return 0;
    }

    if (srvl = srv_lookup("matahari", "tcp", hostdomain)) {
        srv = *srvl;
        for (i = 1; srvl[i]; i++) {
            if (srvl[i]->prio < srv->prio)
                srv = srvl[i];
        }

        printf("\tAccessible QPID Broker: %s\n", srv->name);
        return 0;
    }
    return -1;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: et
 */

