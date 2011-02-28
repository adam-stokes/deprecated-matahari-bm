/* dnssrv.h - Copyright (c) 2011 Red Hat, Inc.
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

#ifndef __DNSSRV_H
#define __DNSSRV_H

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_NAME_LEN 1024

struct srv_reply
{
    int prio;
    int weight;
    int port;
    char name[];
};

struct srv_reply **srv_lookup(char *service,
                              char *protocol,
                              char *domain);

void srv_free(struct srv_reply **srv);

#ifdef __cplusplus
}
#endif

#endif /* __DNSSRV_H */

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: et
 */
