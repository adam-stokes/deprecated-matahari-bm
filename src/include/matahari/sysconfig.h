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

struct conf {
	int scheme;
	int flags;
	char *uri;
	char *data;
	char *query;
	const char *key;
};

// Supported SCHEME types
#define PUPPET      1
#define AUGEAS		2

// Supported FLAGS
#define FORCE		1

extern int mh_sysconfig_run(const char *uri, int flags, int scheme, struct conf *cf);
extern int mh_sysconfig_run_string(const char *string, int flags, int scheme, struct conf *cf);
extern int mh_sysconfig_query(const char *query, const char *data,
                               int flags, int scheme, struct conf *cf);

// scheme specific
extern int mh_sysconfig_run_puppet(struct conf *cf);
extern int mh_sysconfig_run_augeas(struct conf *cf);
extern int mh_sysconfig_query_augeas(struct conf *cf);

#endif // __MH_SYSCONFIG_H__
