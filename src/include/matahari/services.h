#ifndef __MH_SERVICES__
#define __MH_SERVICES__
/* 
 * Copyright (C) 2010 Andrew Beekhof <andrew@beekhof.net>
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

#include <glib.h>
#include <stdio.h>
#include "matahari/mainloop.h"

/* TODO: Autodetect these two in CMakeList.txt */
#define OCF_ROOT "/usr/lib/ocf"
#define LSB_ROOT "/etc/init.d"

enum ocf_exitcode {
	OCF_PENDING = -1,
	OCF_OK = 0,
	OCF_UNKNOWN_ERROR = 1,
	OCF_INVALID_PARAM = 2,
	OCF_UNIMPLEMENT_FEATURE = 3,
	OCF_INSUFFICIENT_PRIV = 4,
	OCF_NOT_INSTALLED = 5,
	OCF_NOT_CONFIGURED = 6,
	OCF_NOT_RUNNING = 7,
	OCF_RUNNING_MASTER = 8,
	OCF_FAILED_MASTER = 9,
};

enum op_status {
	LRM_OP_PENDING = -1,
	LRM_OP_DONE,
	LRM_OP_CANCELLED,
	LRM_OP_TIMEOUT,
	LRM_OP_NOTSUPPORTED,
	LRM_OP_ERROR
};

typedef struct svc_action_private_s svc_action_private_t;
typedef struct svc_action_s 
{
	char *id;
	char *rsc;
	char *action;
	int   interval;

	char *rclass;
	char *provider;
	char *agent;

	int         timeout;
	GHashTable *params;

	int rc;
	int pid;
	int status;
	
	char          *stderr_data;
	char          *stdout_data;

	svc_action_private_t *opaque;
	
} svc_action_t;

extern GList *get_directory_list(const char *root, gboolean files);

extern GList *services_list(void);
extern GList *resources_list_ocf_providers(void);
extern GList *resources_list_ocf_agents(const char *provider);

extern svc_action_t *services_action_create(
    const char *name, const char *action, int interval /* ms */, int timeout /* ms */);

/* After the call, 'params' is owned, and later free'd by the svc_action_t result */
extern svc_action_t *resources_action_create(
    const char *name, const char *provider, const char *agent,
    const char *action, int interval /* ms */, int timeout /* ms */, GHashTable *params);

extern void services_action_free(svc_action_t *op);

extern gboolean services_action_sync(svc_action_t* op);
extern gboolean services_action_async(svc_action_t* op, void (*action_callback)(svc_action_t*));

extern gboolean services_action_cancel(const char *name, const char *action, int interval);

static inline enum ocf_exitcode resources_convert_lsb_exitcode(char *action, int lsb) 
{
    if(lsb == OCF_NOT_INSTALLED) {
	return OCF_NOT_INSTALLED;

    } else if(action != NULL && strcmp("status", action) == 0) {
	switch(lsb) {
	    case 0: return OCF_OK;            /* LSB_STATUS_OK */
	    case 1: return OCF_NOT_RUNNING;   /* LSB_STATUS_VAR_PID */
	    case 2: return OCF_NOT_RUNNING;   /* LSB_STATUS_VAR_LOCK */
	    case 3: return OCF_NOT_RUNNING;   /* LSB_STATUS_STOPPED */
	    case 4: return OCF_UNKNOWN_ERROR; /* LSB_STATUS_UNKNOWN */
	}
    }
    
    return OCF_UNKNOWN_ERROR;
}
#endif
