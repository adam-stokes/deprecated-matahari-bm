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

#ifndef WIN32
#include "config.h"
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <glib.h>
#include <curl/curl.h>
#include "matahari/logging.h"
#include "matahari/sysconfig.h"
#include "matahari/utilities.h"
#include "sysconfig_private.h"


MH_TRACE_INIT_DATA(mh_sysconfig);

static int
sysconfig_os_download(const char *uri, FILE *fp)
{
    CURL *curl;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, uri);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        res = curl_easy_perform(curl);

        if (CURLE_OK != res) {
            return -1;
        }
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
    return 0;
}


static int
sysconfig_os_run_puppet(const char *uri, const char *data, const char *key)
{
    gboolean ret;
    GError *error = NULL;
    char fmt_error[1024];
    gchar *cmd[4];
    char filename[PATH_MAX];
    int fd;
    FILE *fp;

    if (uri != NULL) {
        snprintf(filename, sizeof(filename), "%s", "puppet_conf_XXXXXX");
        fd = mkstemp(filename);
        if (fd < 0) {
            return -1;
        }
        fp = fdopen(fd, "w+b");
        if (fp == NULL) {
            close(fd);
            return -1;
        }
        if ((sysconfig_os_download(uri, fp)) != 0) {
            fclose(fp);
            return -1;
        }
        fclose(fp);
    } else if (data != NULL) {
        snprintf(filename, sizeof(filename), "%s", "puppet_conf_blob");
        g_file_set_contents(filename, data, strlen(data), NULL);
    } else {
        return -1;
    }

    cmd[0] = "puppet";
    cmd[1] = "apply";
    cmd[2] = filename;
    cmd[3] = NULL;
    mh_info("Running %s %s", cmd[0], cmd[1]);
    ret = g_spawn_async(NULL, cmd, NULL, G_SPAWN_SEARCH_PATH,
            NULL, NULL, NULL, &error);
    if (ret == FALSE) {
        snprintf(fmt_error, sizeof(fmt_error), "ERROR\n%s", error->message);
        if (mh_sysconfig_set_configured(key, fmt_error) == FALSE) {
            mh_err("Unable to write to file.");
        }
        g_error_free(error);
        return -1;
    }
    if (mh_sysconfig_set_configured(key, "OK") == FALSE) {
        mh_err("Unable to write to file.");
        return -1;
    }
    return 0;
}

static int
sysconfig_os_run_augeas(const char *query, const char *data, const char *key)
{
    mh_warn("not implemented\n");
    return -1;
}

static const char *
sysconfig_os_query_augeas(const char *query)
{
    const char *data = NULL;
    mh_warn("not implemented\n");
    return data;
}

int
sysconfig_os_run_uri(const char *uri, uint32_t flags, const char *scheme,
        const char *key)
{
    int rc = 0;

    if (mh_sysconfig_is_configured(key) == FALSE || (flags & MH_SYSCONFIG_FLAG_FORCE)) {
        if (strcasecmp(scheme, "puppet") == 0) {
            rc = sysconfig_os_run_puppet(uri, NULL, key);
        } else if (strcasecmp(scheme, "augeas") == 0) {
            rc = sysconfig_os_run_augeas(uri, NULL, key);
        } else {
            rc = -1;
        }
    }
    return rc;
}

int
sysconfig_os_run_string(const char *data, uint32_t flags, const char *scheme,
        const char *key)
{
    int rc = 0;

    if (mh_sysconfig_is_configured(key) == FALSE || (flags & MH_SYSCONFIG_FLAG_FORCE)) {
        if (strcasecmp(scheme, "puppet") == 0) {
            rc = sysconfig_os_run_puppet(NULL, data, key);
        } else {
            rc = -1;
        }
    }
    return rc;
}

const char *
sysconfig_os_query(const char *query, uint32_t flags, const char *scheme)
{
    const char *data = NULL;

    if (strcasecmp(scheme, "augeas") == 0) {
        data = sysconfig_os_query_augeas(query);
    }

    return data;
}
