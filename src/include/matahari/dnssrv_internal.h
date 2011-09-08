/* dnssrv_internal.h - Copyright (c) 2011 Red Hat, Inc.
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
 *
 * The functions in this file are exposed for the purposes of using them
 * from unit tests.  They're not intended to be used as a part of the
 * public API.  If you have a reason to use these, please contact the
 * maintainers of the Matahari project.
 */

#ifndef __MH_DNSSRV_INTERNAL_H__
#define __MH_DNSSRV_INTERNAL_H__

/**
 * Add a record to a collection of DNS SRV records
 *
 * \param[in] records DNS SRV record collection.
 * \param[in] host the hostname in the record
 * \param[in] port the port in the record
 * \param[in] priority the priority in the record
 * \param[in] weight the weight in the record
 *
 * \return the new head of the records list.
 */
GList *
mh_dnssrv_add_record(GList *records, const char *host, uint16_t port,
                     uint16_t priority, uint16_t weight) G_GNUC_WARN_UNUSED_RESULT;

/**
 * Sort a collection of DNS SRV records
 *
 * This is only needed if a collection is built manually by directly calling
 * mh_dnssrv_add_record().  It is not normally needed.
 *
 * \param[in] records the collection of records to sort.
 *
 * \return the new head of the records list.
 */
GList *
mh_dnssrv_records_sort(GList *records) G_GNUC_WARN_UNUSED_RESULT;

#endif /* __MH_DNSSRV_INTERNAL_H__ */
