/* sysconfig.h - Copyright (C) 2011 Red Hat, Inc.
 * Written by Adam Stokes <astokes@fedoraproject.org>
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
 * \file
 * \brief Config API
 * \ingroup coreapi
 */

#ifndef __MH_SYSCONFIG_H__
#define __MH_SYSCONFIG_H__

#include <stdint.h>
/*! Supported FLAGS */
#define MH_SYSCONFIG_FLAG_FORCE    (1 << 0)

/**
 * Download and process URI for configuration
 *
 * \param[in] uri the url of configuration item
 * \param[in] flags flags used
 * \param[in] scheme the type of configuration i.e. puppet
 *
 * \return 0 for success -1 for fail
 */
extern int mh_sysconfig_run_uri(const char *uri, uint32_t flags, const char *scheme);

/**
 * Process a text blob
 *
 * \param[in] string the text blob to process, accepts a range of data from XML to Puppet classes
 * \param[in] flags flags used
 * \param[in] scheme the type of configuration i.e. puppet
 *
 * \return 0 for success -1 for fail
 */
extern int mh_sysconfig_run_string(const char *string, uint32_t flags, const char *scheme);

/**
 * Query against a configuration object on the system
 *
 * \param[in] query the query command used
 * \param[in] flags flags used
 * \param[in] scheme the type of configuration i.e. puppet
 * \param[out] data the data found from query parameter
 *
 * \return data the result found from query
 */
extern const char *
mh_sysconfig_query(const char *query, uint32_t flags, const char *scheme);

#endif // __MH_SYSCONFIG_H__
