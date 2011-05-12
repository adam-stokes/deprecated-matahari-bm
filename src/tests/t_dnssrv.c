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

#include <glib.h>
#include "dnssrv_private.h"

int main(int argc, char **argv)
{
    struct srv_reply **srvl = NULL, *srv = NULL;
    int i;

    if (srvl = srv_lookup("qpid-broker", "tcp", "qpid-broker:49000")) {
        srv = *srvl;
        for (i = 1; srvl[i]; i++) {
            if (srvl[i]->prio < srv->prio)
                srv = srvl[i];
        }

        fprintf("\tAccessible QPID Broker: %s\n", srv->name);
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

