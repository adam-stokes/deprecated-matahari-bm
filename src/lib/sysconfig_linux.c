/* sysconfig_linux.c - Copyright (C) 2011 Red Hat, Inc.
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

#include "config.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <glib.h>
#include <curl/curl.h>

#ifdef HAVE_AUGEAS
#include <augeas.h>
#endif

#include "matahari/logging.h"
#include "matahari/utilities.h"
#include "matahari/services.h"
#include "matahari/sysconfig.h"

#include "utilities_private.h"
#include "sysconfig_private.h"

MH_TRACE_INIT_DATA(mh_sysconfig);

struct action_data {
    char *key;
    char *filename;
    mh_sysconfig_result_cb result_cb;
    void *cb_data;
};

static void
action_data_free(struct action_data *action_data)
{
    free(action_data->key);
    free(action_data->filename);
    free(action_data);
}

/**
 * \internal
 * \note This function is not thread-safe.
 */
static enum mh_result
sysconfig_os_download(const char *uri, FILE *fp)
{
    CURL *curl;
    CURLcode curl_res;
    long response = 0;
    enum mh_result res;

    if ((res = mh_curl_init()) != MH_RES_SUCCESS) {
        return res;
    }

    if (!(curl = curl_easy_init())) {
        return MH_RES_OTHER_ERROR;
    }

    curl_res = curl_easy_setopt(curl, CURLOPT_URL, uri);
    if (curl_res != CURLE_OK) {
        mh_warn("curl_easy_setopt of URI '%s' failed. (%d)", uri, curl_res);
        res = MH_RES_OTHER_ERROR;
        goto return_cleanup;
    }

    curl_res = curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    if (curl_res != CURLE_OK) {
        mh_warn("curl_easy_setopt of WRITEDATA '%p' failed. (%d)", fp, curl_res);
        res = MH_RES_OTHER_ERROR;
        goto return_cleanup;
    }

    curl_res = curl_easy_perform(curl);
    if (curl_res != CURLE_OK) {
        mh_warn("curl request for URI '%s' failed. (%d)", uri, curl_res);
        res = MH_RES_DOWNLOAD_ERROR;
        goto return_cleanup;
    }

    if (!strncasecmp(uri, "http", 4) || !strncasecmp(uri, "ftp", 3)) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response);
        if (curl_res != CURLE_OK) {
            mh_warn("curl_easy_getinfo for RESPONSE_CODE failed. (%d)", curl_res);
            res = MH_RES_DOWNLOAD_ERROR;
            goto return_cleanup;
        }
        if (response < 200 || response > 299) {
            mh_warn("curl request for URI '%s' got response %ld", uri, response);
            res = MH_RES_DOWNLOAD_ERROR;
        }
    }

return_cleanup:
    curl_easy_cleanup(curl);

    return res;
}

static void
action_cb(svc_action_t *action)
{
    struct action_data *action_data = action->cb_data;
    char buf[32] = "OK";

    if (action->rc) {
        snprintf(buf, sizeof(buf), "FAILED\n%d", action->rc);
    }

    if (mh_sysconfig_set_configured(action_data->key, buf) != MH_RES_SUCCESS) {
        mh_err("Unable to write to key file '%s'", action_data->key);
    }

    action_data->result_cb(action_data->cb_data, action->rc);

    unlink(action_data->filename);

    action_data_free(action_data);
    action->cb_data = NULL;
}

/**
 * \internal
 * \brief Check the installed version of puppet.
 *
 * \param[out] use_apply whether to use "puppet <foo>" or "puppet apply <foo>"
 *
 * \retval 0 success
 * \retval -1 failed to find puppet at all
 */
static int
check_puppet(int *use_apply)
{
    gboolean spawn_res;
    gchar *argv[] = {
        "puppet", "--version", NULL,
    };
    gchar *out = NULL;
    gchar *out_copy = NULL;
    GError *error = NULL;
    char *dot;
    int res = 0;
    unsigned int major;

    spawn_res = g_spawn_sync(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL,
                             &out, NULL, NULL, &error);

    if (spawn_res == FALSE) {
        mh_err("Failed to check puppet version: (%d) %s", error->code, error->message);
        g_error_free(error);
        return -1;
    }

    out_copy = g_strdup(out);

    if (!(dot = strchr(out_copy, '.'))) {
        mh_err("Unexpected output from 'puppet --version': '%s'", out);
        res = -1;
        goto return_cleanup;
    }

    *dot = '\0';

    if (sscanf(out_copy, "%u", &major) != 1) {
        mh_err("Failed to parse outpuet from 'puppet --version': '%s'", out);
        res = -1;
    }

    *use_apply = (major >= 2) ? 1 : 0;

return_cleanup:
    g_free(out);
    g_free(out_copy);

    return res;
}

static enum mh_result
run_puppet(const char *uri, const char *data, const char *key,
           mh_sysconfig_result_cb result_cb, void *cb_data)
{
    const char *args[3];
    char filename[PATH_MAX];
    svc_action_t *action = NULL;
    struct action_data *action_data = NULL;
    int use_apply = 0;
    enum mh_result res = MH_RES_SUCCESS;

    if (check_puppet(&use_apply)) {
        return MH_RES_BACKEND_ERROR;
    }

    if (uri) {
        int fd;
        FILE *fp;

        snprintf(filename, sizeof(filename), "%s", "puppet_conf_XXXXXX");

        fd = mkstemp(filename);
        if (fd < 0) {
            return MH_RES_OTHER_ERROR;
        }

        fp = fdopen(fd, "w+b");
        if (fp == NULL) {
            close(fd);
            unlink(filename);
            return MH_RES_OTHER_ERROR;
        }

        if ((res = sysconfig_os_download(uri, fp)) != 0) {
            fclose(fp);
            unlink(filename);
            return res;
        }

        fclose(fp);
    } else if (data) {
        snprintf(filename, sizeof(filename), "puppet_conf_%u", g_random_int());
        g_file_set_contents(filename, data, strlen(data), NULL);
    } else {
        return MH_RES_INVALID_ARGS;
    }

    if (use_apply) {
        args[0] = "apply";
        args[1] = filename;
        args[2] = NULL;
    } else {
        args[0] = filename;
        args[1] = NULL;
    }

    if (!(action = mh_services_action_create_generic("puppet", args))) {
        res = MH_RES_BACKEND_ERROR;
        goto return_failure;
    }

    action_data = calloc(1, sizeof(*action_data));
    action_data->key = strdup(key);
    action_data->filename = strdup(filename);
    action_data->result_cb = result_cb;
    action_data->cb_data = cb_data;

    action->cb_data = action_data;
    action->id = strdup("puppet");

    mh_info("Running puppet %s%s", use_apply ? "apply " : "", filename);

    if (services_action_async(action, action_cb) == FALSE) {
        res = MH_RES_BACKEND_ERROR;
        goto return_failure;
    }

    return MH_RES_SUCCESS;

return_failure:
    if (action) {
        services_action_free(action);
        action = NULL;
    }

    if (action_data) {
        action_data_free(action_data);
        action_data = NULL;
    }

    if (mh_sysconfig_set_configured(key, "ERROR") != MH_RES_SUCCESS) {
        mh_err("Unable to write to file.");
    }

    unlink(filename);

    return res;
}

static enum mh_result
run_augeas(const char *uri, const char *data, const char *key,
           mh_sysconfig_result_cb result_cb, void *cb_data)
{
#ifdef HAVE_AUGEAS
    int fd;
    FILE *fp;
    char *text = NULL;
    char filename[PATH_MAX];
    GError *err = NULL;
    augeas *aug;
    int result;
    char *value = NULL, *result_str;
    enum mh_result res = MH_RES_SUCCESS;

    if (uri) {
        snprintf(filename, sizeof(filename), "%s", "augeas_conf_XXXXXX");

        fd = mkstemp(filename);
        if (fd < 0) {
            mh_err("Unable to create temporary file");
            return MH_RES_OTHER_ERROR;
        }

        fp = fdopen(fd, "w+b");
        if (fp == NULL) {
            close(fd);
            unlink(filename);
            mh_err("Unable to open temporary file");
            return MH_RES_OTHER_ERROR;
        }

        if ((res = sysconfig_os_download(uri, fp)) != 0) {
            fclose(fp);
            unlink(filename);
            mh_err("Unable to download file from uri %s", uri);
            return res;
        }

        fclose(fp);

        if (!g_file_get_contents(filename, &text, NULL, &err)) {
            mh_err("Unable to read downloaded file: %s", err ? err->message : "Unknown error");
            unlink(filename);
            return MH_RES_DOWNLOAD_ERROR;
        }

        unlink(filename);

    } else if (data) {
        text = g_strdup(data);
    } else {
        mh_err("No uri/data provided for augeas");
        return MH_RES_INVALID_ARGS;
    }

    snprintf(filename, sizeof(filename), "%s", "augeas_conf_XXXXXX");

    fd = mkstemp(filename);
    if (fd < 0) {
        mh_err("Unable to create temporary file");
        return MH_RES_OTHER_ERROR;
    }

    fp = fdopen(fd, "w+b");
    if (fp == NULL) {
        close(fd);
        unlink(filename);
        mh_err("Unable to open temporary file");
        return MH_RES_OTHER_ERROR;
    }

    aug = aug_init("", "", AUG_SAVE_BACKUP);
    if (!aug) {
        g_free(text);
        fclose(fp);
        unlink(filename);
        mh_err("Unable to initialize augeas");
        return MH_RES_BACKEND_ERROR;
    }

    result = aug_srun(aug, fp, text);

    mh_info("run_augeas for key \"%s\" exited with status %d and data \"%s\"", key, result, text);

    aug_close(aug);

    g_free(text);
    fclose(fp);

    if (!g_file_get_contents(filename, &value, NULL, &err)) {
        mh_err("Unable to read augeas results: %s", err->message);
        unlink(filename);
        return MH_RES_BACKEND_ERROR;
    }

    if (result < 0) {
        asprintf(&result_str, "FAILED\n%d\n%s", result, value);
        mh_sysconfig_set_configured(key, result_str);
        result_cb(cb_data, MH_RES_SUCCESS);
    } else {
        asprintf(&result_str, "OK\n%s", value);
        mh_sysconfig_set_configured(key, result_str);
        result_cb(cb_data, MH_RES_SUCCESS);
    }

    free(result_str);
    unlink(filename);

    return MH_RES_SUCCESS;
#else /* HAVE_AUGEAS */
    return MH_RES_NOT_IMPLEMENTED;
#endif /* HAVE_AUGEAS */
}

static char *
sysconfig_os_query_augeas(const char *query)
{
#ifdef HAVE_AUGEAS
    char *data = NULL;
    const char *value = NULL;
    augeas *aug = aug_init("", "", 0);
    aug_get(aug, query, &value);
    if (value)
        data = strdup(value);
    aug_close(aug);
    return data;
#else /* HAVE_AUGEAS */
    return NULL;
#endif /* HAVE_AUGEAS */
}

enum mh_result
sysconfig_os_run_uri(const char *uri, uint32_t flags, const char *scheme,
        const char *key, mh_sysconfig_result_cb result_cb, void *cb_data)
{
    enum mh_result rc = MH_RES_SUCCESS;

    if (mh_sysconfig_is_configured(key) && !(flags & MH_SYSCONFIG_FLAG_FORCE)) {
        /*
         * Already configured and not being forced.  Report success now.
         */
        result_cb(cb_data, rc);
        return rc;
    }

    if (strcasecmp(scheme, "puppet") == 0) {
        rc = run_puppet(uri, NULL, key, result_cb, cb_data);
    } else if (strcasecmp(scheme, "augeas") == 0) {
        rc = run_augeas(uri, NULL, key, result_cb, cb_data);
    } else {
        rc = MH_RES_INVALID_ARGS;
    }

    return rc;
}

enum mh_result
sysconfig_os_run_string(const char *string, uint32_t flags, const char *scheme,
        const char *key, mh_sysconfig_result_cb result_cb, void *cb_data)
{
    enum mh_result rc = MH_RES_SUCCESS;

    if (mh_sysconfig_is_configured(key) && !(flags & MH_SYSCONFIG_FLAG_FORCE)) {
        /*
         * Already configured and not being forced.  Report success now.
         */
        result_cb(cb_data, rc);
        return rc;
    }

    if (!strcasecmp(scheme, "puppet")) {
        rc = run_puppet(NULL, string, key, result_cb, cb_data);
    } else if (strcasecmp(scheme, "augeas") == 0) {
        rc = run_augeas(NULL, string, key, result_cb, cb_data);
    } else {
        rc = MH_RES_INVALID_ARGS;
    }

    return rc;
}

char *
sysconfig_os_query(const char *query, uint32_t flags, const char *scheme)
{
    char *data = NULL;

    if (strcasecmp(scheme, "augeas") == 0) {
        data = sysconfig_os_query_augeas(query);
    }

    return data;
}
