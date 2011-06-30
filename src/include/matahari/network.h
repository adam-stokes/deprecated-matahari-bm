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

/**
 * An opaque type for a network interface
 */
struct mh_network_interface;

/**
 * Flags regarding the state of a network interface.
 */
enum mh_network_interface_flags {
    MH_NETWORK_IF_UP =   (1 << 0),
    MH_NETWORK_IF_DOWN = (1 << 1),
};

/**
 * Get the name of a network interface
 *
 * \param[in] iface the network interface
 *
 * \return the name for the provided network interface
 */
extern const char *
mh_network_interface_get_name(const struct mh_network_interface *iface);

/**
 * Get flags for the state of a network interface
 *
 * \param[in] iface the network interface
 *
 * \return mh_network_interface_flags
 */
uint64_t
mh_network_interface_get_flags(const struct mh_network_interface *iface);

/**
 * Destroy a network interface
 *
 * This destroys an allocated mh_network_interface.  This function should
 * be used in combination with g_list_free_full() on the rest from
 * mh_network_get_interfaces().  For example:
 *
 * \code
 * GList *iface_list;
 *
 * iface_list = mh_network_get_interfaces();
 * ... do stuff with the iface_list ...
 * g_list_free_full(iface_list, mh_network_interface_destroy);
 * \endcode
 */
extern void
mh_network_interface_destroy(gpointer data);

/**
 * Get a list of network interfaces.
 *
 * \note Don't forget to free this list when done with it!  To do so, use
 *       g_list_free_full(iface_list, mh_network_interface_destroy).
 *
 * \return A list of network interfaces.  The data for each list item is a
 *         struct mh_network_interface.
 */
extern GList *
mh_network_get_interfaces(void);

/**
 * Get the IP address of a network interface
 *
 * \param[in] iface network interface name
 * \param[out] buf buffer to hold the IP address
 * \param[in] len length of buf
 *
 * \return buf, for convenience
 */
extern const char *
mh_network_get_ip_address(const char *iface, char *buf, size_t len);

/**
 * Get the MAC address of a network interface
 *
 * \param[in] iface network interface name
 * \param[out] buf buffer to hold the MAC address
 * \param[in] len length of buf
 *
 * \return buf, for convenience
 */
extern const char *
mh_network_get_mac_address(const char *iface, char *buf, size_t len);

extern void mh_network_stop(const char *iface);
extern void mh_network_start(const char *iface);
extern void mh_network_restart(const char *iface);

/**
 * Get the status of a network interface
 *
 * \param[in] iface the network interface
 * \param[out] flags a bit field of flags set that indicate the status of the
 *             interface.  See mh_network_interface_flags for a list of possible
 *             flags.
 *
 * \retval 0 network interface was found
 * \retval non-zero network interface was not found and the value of flags
 *         is undefined.
 */
extern int
mh_network_status(const char *iface, uint64_t *flags);

#endif /* __NETWORK_H */
