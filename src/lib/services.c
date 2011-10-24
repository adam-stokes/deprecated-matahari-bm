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

#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>

#include "matahari/logging.h"
#include "matahari/mainloop.h"
#include "matahari/services.h"
#include "services_private.h"
#include "sigar.h"

MH_TRACE_INIT_DATA(mh_services);

/* TODO: Develop a rollover strategy */

static int operations = 0;
GHashTable *recurring_actions = NULL;

svc_action_t *
services_action_create(const char *name, const char *action, int interval,
                       int timeout)
{
    return resources_action_create(name, "lsb", NULL, name, action, interval, timeout, NULL);
}

svc_action_t *resources_action_create(
    const char *name, const char *standard, const char *provider, const char *agent,
    const char *action, int interval, int timeout, GHashTable *params)
{
    svc_action_t *op;

    /*
     * Do some up front sanity checks before we go off and
     * build the svc_action_t instance.
     */

    if (mh_strlen_zero(name)) {
        mh_err("A service or resource action must have a name.");
        return NULL;
    }

    if (mh_strlen_zero(standard)) {
        mh_err("A service action must have a valid standard.");
        return NULL;
    }

    if (!strcasecmp(standard, "ocf") && mh_strlen_zero(provider)) {
        mh_err("An OCF resource action must have a provider.");
        return NULL;
    }

    if (mh_strlen_zero(agent)) {
        mh_err("A service or resource action must have an agent.");
        return NULL;
    }

    if (mh_strlen_zero(action)) {
        mh_err("A service or resource action must specify an action.");
        return NULL;
    }

    /*
     * Sanity checks passed, proceed!
     */

    op = calloc(1, sizeof(svc_action_t));
    op->opaque = calloc(1, sizeof(svc_action_private_t));
    op->rsc = strdup(name);
    op->action = strdup(action);
    op->interval = interval;
    op->timeout = timeout;
    op->standard = strdup(standard);
    op->agent = strdup(agent);
    op->sequence = ++operations;
    if (asprintf(&op->id, "%s_%s_%d", name, action, interval) == -1) {
        goto return_error;
    }

    if(strcasecmp(standard, "ocf") == 0) {
        op->provider = strdup(provider);
        op->params = params;

        if (asprintf(&op->opaque->exec, "%s/resource.d/%s/%s",
                     OCF_ROOT, provider, agent) == -1) {
            goto return_error;
        }
        op->opaque->args[0] = strdup(op->opaque->exec);
        op->opaque->args[1] = strdup(action);

    } else if(strcasecmp(standard, "lsb") == 0 || strcasecmp(standard, "windows") == 0) {
        services_os_set_exec(op);

    } else if(strcasecmp(standard, "systemd") == 0) {
        char *service;
        op->opaque->exec = strdup(SYSTEMCTL);
        op->opaque->args[0] = strdup(SYSTEMCTL);
        op->opaque->args[1] = strdup(action);
        if (asprintf(&service, "%s.service", agent) == -1) {
            goto return_error;
        }
        op->opaque->args[2] = service;
    } else {
        mh_err("Unknown resource standard: %s", standard);
        services_action_free(op);
        op = NULL;
    }

    return op;

return_error:
    services_action_free(op);

    return NULL;
}

svc_action_t *
mh_services_action_create_generic(const char *exec, const char *args[])
{
    svc_action_t *op;
    unsigned int cur_arg;

    op = calloc(1, sizeof(*op));
    op->opaque = calloc(1, sizeof(svc_action_private_t));

    op->opaque->exec = strdup(exec);
    op->opaque->args[0] = strdup(exec);

    for (cur_arg = 1; args && args[cur_arg - 1]; cur_arg++) {
        op->opaque->args[cur_arg] = strdup(args[cur_arg - 1]);

        if (cur_arg == DIMOF(op->opaque->args) - 1) {
            mh_err("svc_action_t args list not long enough for '%s' execution request.", exec);
            break;
        }
    }

    return op;
}

void
services_action_free(svc_action_t *op)
{
    unsigned int i;

    if (op == NULL) {
        return;
    }

    if (op->opaque->stderr_gsource) {
        mainloop_destroy_fd(op->opaque->stderr_gsource);
        op->opaque->stderr_gsource = NULL;
    }

    if (op->opaque->stdout_gsource) {
        mainloop_destroy_fd(op->opaque->stdout_gsource);
        op->opaque->stdout_gsource = NULL;
    }

    free(op->id);
    free(op->opaque->exec);

    for (i = 0; i < DIMOF(op->opaque->args); i++) {
        free(op->opaque->args[i]);
    }

    free(op->rsc);
    free(op->action);

    free(op->standard);
    free(op->agent);
    free(op->provider);

    free(op->stdout_data);
    free(op->stderr_data);

    if (op->params) {
        g_hash_table_destroy(op->params);
        op->params = NULL;
    }

    free(op);
}

gboolean
services_action_cancel(const char *name, const char *action, int interval)
{
    svc_action_t* op = NULL;
    char id[512];

    snprintf(id, sizeof(id), "%s_%s_%d", name, action, interval);

    if (!(op = g_hash_table_lookup(recurring_actions, id))) {
        return FALSE;
    }

    mh_debug("Removing %s", op->id);
    if (op->opaque->repeat_timer) {
        g_source_remove(op->opaque->repeat_timer);
    }
    services_action_free(op);

    return TRUE;
}

gboolean
services_action_async(svc_action_t* op, void (*action_callback)(svc_action_t *))
{
    if (action_callback) {
        op->opaque->callback = action_callback;
    }

    if (recurring_actions == NULL) {
        recurring_actions = g_hash_table_new_full(g_str_hash, g_str_equal,
                                                  NULL, NULL);
    }

    if (op->interval > 0) {
        g_hash_table_replace(recurring_actions, op->id, op);
    }

    return services_os_action_execute(op, FALSE);
}

gboolean
services_action_sync(svc_action_t* op)
{
    gboolean rc = services_os_action_execute(op, TRUE);
    mh_trace(" > %s_%s_%d: %s = %d", op->rsc, op->action, op->interval,
             op->opaque->exec, op->rc);
    if (op->stdout_data) {
        mh_trace(" >  stdout: %s", op->stdout_data);
    }
    if (op->stderr_data) {
        mh_trace(" >  stderr: %s", op->stderr_data);
    }
    return rc;
}

GList *
get_directory_list(const char *root, gboolean files)
{
    return services_os_get_directory_list(root, files);
}

GList *
services_list(void)
{
    return resources_list_agents("lsb", NULL);
}

GList *
resources_list_standards(void)
{
    GList *standards = NULL;
#ifdef __linux__
    standards = g_list_append(standards, strdup("ocf"));
    standards = g_list_append(standards, strdup("lsb"));
    if (g_file_test(SYSTEMCTL, G_FILE_TEST_IS_REGULAR))
        standards = g_list_append(standards, strdup("systemd"));
#endif
#ifdef WIN32
    standards = g_list_append(standards, strdup("windows"));
#endif
    return standards;
}

GList *
resources_list_providers(const char *standard)
{
    if (strcasecmp(standard, "ocf") == 0) {
        return resources_os_list_ocf_providers();
    }

    return NULL;
}

GList *
resources_list_agents(const char *standard, const char *provider)
{
    if (strcasecmp(standard, "ocf") == 0) {
        return resources_os_list_ocf_agents(provider);

    } else if (strcasecmp(standard, "lsb") == 0
            || strcasecmp(standard, "windows") == 0) {
        return services_os_list();

#ifdef __linux__
    } else if (strcasecmp(standard, "systemd") == 0) {
        return resources_os_list_systemd_services();
#endif
    }

    return NULL;
}
