/* dnssrv_private.h - Copyright (C) 2011 Red Hat, Inc.
 * Written by Russell Bryant <rbryant@redhat.com>
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
 * \brief Platform specific function prototypes.
 *
 * The functions in this header that have a mh_os_ prefix must be implemented
 * by the platform specific host data code.
 *  - dnssrv_linux.c
 *  - dnssrv_windows.c
 */

#ifndef __MH_DNSSRV_PRIVATE_H__
#define __MH_DNSSRV_PRIVATE_H__

#include <inttypes.h>

/**
 * Perform a DNS SRV lookup.
 *
 * \param[in] query DNS SRV lookup, such as _matahari._tcp.matahariproject.org
 *
 * \return head of DNS SRV records list
 */
GList *
mh_os_dnssrv_lookup(const char *query);

#endif /* __MH_DNSSRV_PRIVATE_H__ */
