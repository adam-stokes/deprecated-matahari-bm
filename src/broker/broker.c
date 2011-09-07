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
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include <matahari/logging.h>

#include "broker_os.h"
#include "broker.h"


uint16_t broker_get_port(void)
{
    static uint16_t port = 0;

    if (!port) {
        char *port_env = getenv("MATAHARI_PORT");
        if (port_env) {
            if (sscanf(port_env, "%hu", &port) != 1) {
                mh_warn("Failed to parse MATAHARI_PORT=\"%s\"", port_env);
            }
        }
    }

    return port ? port : MATAHARI_PORT;
}

static char **broker_args(int argc, char * const argv[])
{
    int i = 0, o = 0;
    char portarg[13];
    char **newargs = malloc(sizeof(char *) * (argc + 2 + 1));

    if (argc) {
        newargs[o++] = strdup(argv[i++]);
    }

    snprintf(portarg, sizeof(portarg), "--port=%hu", broker_get_port());
    newargs[o++] = strdup(portarg);
    newargs[o++] = strdup("--data-dir=" LOCAL_STATE_DIR "/lib/matahari");

    while (i < argc) {
        newargs[o++] = strdup(argv[i++]);
    }

    newargs[o] = NULL;

    return newargs;
}

static void broker_args_free(char **args)
{
    char **p = args;

    while (*args) {
        free(*args++);
    }

    free(p);
}

int main(int argc, char *argv[])
{
    char **arglist = broker_args(argc, argv);

    int ret = broker_os_start_broker(arglist);

    broker_args_free(arglist);

    return ret;
}

