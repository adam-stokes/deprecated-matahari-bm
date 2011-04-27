/* network.h - Copyright (c) 2010 Red Hat, Inc.
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

/**
 * \file
 * \brief Network API
 * \ingroup coreapi
 */

#ifndef __NETWORK_H
#define __NETWORK_H

#include <glib.h>
#include <stdint.h>

extern GList *mh_network_get_interfaces(void);
extern const char *mh_network_get_ip_address(const char *iface);
extern const char *mh_network_get_mac_address(const char *iface);

extern void mh_network_stop(const char *iface);
extern void mh_network_start(const char *iface);
extern void mh_network_restart(const char *iface);
extern void mh_network_status(const char *iface, uint64_t *flags);

#endif /* __NETWORK_H */
