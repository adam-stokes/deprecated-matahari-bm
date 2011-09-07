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

/**
 * \file
 * \brief Services API
 * \ingroup coreapi
 */

#ifndef __MH_SERVICES__
#define __MH_SERVICES__

#include <glib.h>
#include <stdio.h>
#include "matahari/mainloop.h"

/* TODO: Autodetect these two in CMakeList.txt */
#define OCF_ROOT "/usr/lib/ocf"
#define LSB_ROOT "/etc/init.d"

enum lsb_exitcode {
    LSB_OK = 0,
    LSB_UNKNOWN_ERROR = 1,
    LSB_INVALID_PARAM = 2,
    LSB_UNIMPLEMENT_FEATURE = 3,
    LSB_INSUFFICIENT_PRIV = 4,
    LSB_NOT_INSTALLED = 5,
    LSB_NOT_CONFIGURED = 6,
    LSB_NOT_RUNNING = 7,

    /* 150-199	reserved for application use */
    LSB_SIGNAL        = 194,
    LSB_NOT_SUPPORTED = 195,
    LSB_PENDING       = 196,
    LSB_CANCELLED     = 197,
    LSB_TIMEOUT       = 198,
    LSB_OTHER_ERROR   = 199,
};

/* The return codes for the status operation are not the same for other
 * operatios - go figure */
enum lsb_status_exitcode {
    LSB_STATUS_OK = 0,
    LSB_STATUS_VAR_PID = 1,
    LSB_STATUS_VAR_LOCK = 2,
    LSB_STATUS_NOT_RUNNING = 3,
    LSB_STATUS_NOT_INSTALLED = 4,

    /* 150-199 reserved for application use */
    LSB_STATUS_SIGNAL        = 194,
    LSB_STATUS_NOT_SUPPORTED = 195,
    LSB_STATUS_PENDING       = 196,
    LSB_STATUS_CANCELLED     = 197,
    LSB_STATUS_TIMEOUT       = 198,
    LSB_STATUS_OTHER_ERROR   = 199,
};

enum ocf_exitcode {
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

    /* 150-199	reserved for application use */
    OCF_SIGNAL        = 194,
    OCF_NOT_SUPPORTED = 195,
    OCF_PENDING       = 196,
    OCF_CANCELLED     = 197,
    OCF_TIMEOUT       = 198,
    OCF_OTHER_ERROR   = 199, /* Keep the same codes as LSB */
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

    char *standard;
    char *provider;
    char *agent;

    int         timeout;
    GHashTable *params;

    int rc;
    int pid;
    int status;
    int sequence;
    int expected_rc;

    char          *stderr_data;
    char          *stdout_data;

    /**
     * Data stored by the creator of the action.
     *
     * This may be used to hold data that is needed later on by a callback,
     * for example.
     */
    void *cb_data;

    svc_action_private_t *opaque;

} svc_action_t;

/**
 * Get a list of files or directories in a given path
 *
 * \param[in] root full path to a directory to read
 * \param[in] files true to get a list of files, false for a list of directories
 *
 * \return a list of what was found.  The list items are gchar *.  This list _must_
 *         be destroyed using g_list_free_full(list, free).
 */
extern GList *
get_directory_list(const char *root, gboolean files);

/**
 * Get a list of services
 *
 * \return a list of services.  The list items are gchar *.  This list _must_
 *         be destroyed using g_list_free_full(list, free).
 */
extern GList *
services_list(void);

/**
 * Get a list of providers
 *
 * \param[in] the standard for providers to check for (such as "ocf")
 *
 * \return a list of providers.  The list items are gchar *.  This list _must_
 *         be destroyed using g_list_free_full(list, free).
 */
extern GList *
resources_list_providers(const char *standard);

/**
 * Get a list of resource agents
 *
 * \param[in] the standard for research agents to check for
 *            (such as "ocf", "lsb", or "windows")
 *
 * \return a list of resource agents.  The list items are gchar *.  This list _must_
 *         be destroyed using g_list_free_full(list, free).
 */
extern GList *
resources_list_agents(const char *standard, const char *provider);

extern svc_action_t *services_action_create(
    const char *name, const char *action, int interval /* ms */, int timeout /* ms */);

/* After the call, 'params' is owned, and later free'd by the svc_action_t result */
extern svc_action_t *resources_action_create(
    const char *name, const char *standard, const char *provider, const char *agent,
    const char *action, int interval /* ms */, int timeout /* ms */, GHashTable *params);

extern void services_action_free(svc_action_t *op);

extern gboolean services_action_sync(svc_action_t *op);
extern gboolean services_action_async(svc_action_t *op,
                                      void (*action_callback)(svc_action_t *));

extern gboolean services_action_cancel(const char *name, const char *action,
                                       int interval);

static inline enum ocf_exitcode
services_get_ocf_exitcode(char *action, int lsb_exitcode)
{
    if (action != NULL && strcmp("status", action) == 0) {
        switch (lsb_exitcode) {
        case LSB_STATUS_OK:            return OCF_OK;
        case LSB_STATUS_VAR_PID:       return OCF_NOT_RUNNING;
        case LSB_STATUS_VAR_LOCK:      return OCF_NOT_RUNNING;
        case LSB_STATUS_NOT_RUNNING:   return OCF_NOT_RUNNING;
        case LSB_STATUS_NOT_INSTALLED: return OCF_UNKNOWN_ERROR;
        default:                       return OCF_UNKNOWN_ERROR;
        }

    } else if (lsb_exitcode > LSB_NOT_RUNNING) {
        return OCF_UNKNOWN_ERROR;
    }

    /* For non-status operations, the LSB and OCF share error code meaning
     * for rc <= 7 */
    return (enum ocf_exitcode)lsb_exitcode;
}

#endif /* __MH_SERVICES__ */
