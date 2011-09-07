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

/**
 * @file broker_os.h
 * Interface to the OS-specific broker configuration code.
 */

#ifndef MH_BROKER_OS_H
#define MH_BROKER_OS_H

/**
 * A route between two Qpid brokers.
 */
struct mh_qpid_route {
    const char *dest;      /**< The route destination */
    const char *src;       /**< The route source */
    const char *exchange;  /**< The exchange */
};


/**
 * Start the broker with the supplied argument list.
 * @param args a list of args to pass to the broker. The last item in the list
 * must be NULL.
 */
int broker_os_start_broker(char * const args[]);

/**
 * Add a route between two Qpid brokers.
 * @param route pointer to the route to add
 * @return -1 on error, otherwise the result of the external route add command.
 */
int broker_os_add_qpid_route(const struct mh_qpid_route *route);

#endif
