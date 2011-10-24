/* Copyright (C) 2011 Red Hat, Inc.
 * Written by Zane Bitter <zbitter@redhat.com>
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include <matahari/logging.h>
#include <matahari/utilities.h>

#include "broker_federation.h"
#include "broker_os.h"


MH_TRACE_INIT_DATA(mh_broker);

int broker_os_start_broker(char * const args[])
{
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork()");
        return errno;
    }

    if (pid) {
        // exec() in parent process so that qpidd keeps the initial pid.
        // Note that this means the child will become a zombie. That's just the
        // way it goes.

        // execvp() will find the path to qpidd from $PATH.
        execvp("qpidd", args);

        // It's an error if we get here
        perror("exec()");
        return errno;
    } else {
        // Configure federated brokers in the child process
        broker_federation_configure();
        return 0;
    }
}

int broker_os_add_qpid_route_link(const char *local, const char *remote)
{
    char cmd[1024];
    int ret;

    snprintf(cmd, sizeof(cmd), "qpid-route link add %s untrusted/untrusted@%s PLAIN",
             local, remote);

    ret = system(cmd);

    if (ret < 0) {
        perror("system()");
    }
    return ret;
}

int broker_os_add_qpid_route(const struct mh_qpid_route *route)
{
    char cmd[1024];
    int ret;

    if (route->aggregate && route->srclocal) {
        snprintf(cmd, sizeof(cmd), "qpid-route --src-local route add %s %s %s %s",
                 route->dest, route->src, route->exchange, route->route_key);
    } else if (route->aggregate) {
        snprintf(cmd, sizeof(cmd), "qpid-route route add %s %s %s %s",
                 route->dest, route->src, route->exchange, route->route_key);
    } else {
        snprintf(cmd, sizeof(cmd), "qpid-route --timeout=5 dynamic add %s %s %s",
                 route->dest, route->src, route->exchange);
    }

    ret = system(cmd);

    if (ret < 0) {
        perror("system()");
    }
    return ret;
}

