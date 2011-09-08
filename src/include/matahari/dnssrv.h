/* dnssrv.h - Copyright (c) 2011 Red Hat, Inc.
 * Written by Adam Stokes <astokes@fedoraproject.org>
 * Written by Russell Bryant <rbryant@redhat.com>
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
 * \brief DNS SRV Lookup
 * \ingroup coreapi
 */

#ifndef __MH_DNSSRV_H__
#define __MH_DNSSRV_H__

#include <sys/types.h>
#include <inttypes.h>
#include <glib.h>

struct mh_dnssrv_record;

/**
 * Perform a DNS SRV lookup.
 *
 * \param[in] query srv record query i.e. "_matahari._tcp.matahariproject.org
 *
 * \return a sorted list of records (mh_dnssrv_record).  This list must be freed
 * by using glib_list_free_full(records, mh_dnssrv_record_free).
 */
GList *
mh_dnssrv_lookup(const char *query) G_GNUC_WARN_UNUSED_RESULT;

/**
 * Do a DNS SRV lookup and get a single result.
 *
 * This function can be used to do a DNS SRV lookup when all you care about is
 * the highest priority result.  All other results are ignored.
 *
 * \param[in] query srv record query i.e. "_matahari._tcp.matahariproject.org
 * \param[out] host output parameter to hold the hostname
 * \param[in] host_len length of the host output buffer
 * \param[out] port output parameter for the port number
 *
 * \retval 0 success
 * \retval -1 failure, host and port contents are undefined
 */
int
mh_dnssrv_lookup_single(const char *query, char *host, size_t host_len, uint16_t *port);

/**
 * Get the hostname in an SRV record
 *
 * \param[in] record the SRV record
 *
 * \return the hostname
 */
const char *
mh_dnssrv_record_get_host(const struct mh_dnssrv_record *record);

/**
 * Get the port in an SRV record
 *
 * \param[in] record the SRV record
 *
 * \return the port
 */
uint16_t
mh_dnssrv_record_get_port(const struct mh_dnssrv_record *record);

/**
 * Get the priority in an SRV record
 *
 * \param[in] record the SRV record
 *
 * \return the port
 */
uint16_t
mh_dnssrv_record_get_priority(const struct mh_dnssrv_record *record);

/**
 * Get the weigh5 in an SRV record
 *
 * \param[in] record the SRV record
 *
 * \return the weight
 */
uint16_t
mh_dnssrv_record_get_weight(const struct mh_dnssrv_record *record);

/**
 * Destructor for struct mh_dnssrv_record.
 *
 * This is to be used with g_list_free_full when freeing a list returned
 * by mh_dnssrv_lookup.
 */
void
mh_dnssrv_record_free(gpointer data);

#endif /* __MH_DNSSRV_H__ */
