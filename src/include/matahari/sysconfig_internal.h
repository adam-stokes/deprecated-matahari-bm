/* sysconfig_internal.h - Copyright (C) 2011 Red Hat, Inc.
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
 * \brief Sysconfig private functions
 *
 * The functions in this file are exposed for the purposes of using them
 * from unit tests.  They're not intended to be used as a part of the
 * public API.  If you have a reason to use these, please contact the
 * maintainers of the Matahari project.
 */

#ifndef __MH_SYSCONFIG_INTERNAL_H__
#define __MH_SYSCONFIG_INTERNAL_H__

/**
 * Set the directory used to store data about keys
 *
 * This is primarily intended to be used in unit test code.  It should not
 * be needed by any production usage of this library.
 *
 * \param[in] path directory to store key info
 *
 * \note The return of this routine must be freed with free()
 *
 * \return nothing
 */
void
mh_sysconfig_keys_dir_set(const char *path);

#endif /* __MH_SYSCONFIG_INTERNAL_H__ */
