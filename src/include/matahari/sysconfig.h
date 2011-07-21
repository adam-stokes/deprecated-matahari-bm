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
// Supported FLAGS
#define SYSCONFIG_FLAG_FORCE    (1 << 0)

extern int mh_sysconfig_run_uri(const char *uri, uint32_t flags, const char *scheme);
extern int mh_sysconfig_run_string(const char *string, uint32_t flags, const char *scheme);
extern int mh_sysconfig_query(const char *query, uint32_t flags, const char *scheme,
        const char *data);
#endif // __MH_SYSCONFIG_H__
