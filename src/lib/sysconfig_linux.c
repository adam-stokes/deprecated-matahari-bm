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
#include <stddef.h>
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <curl/curl.h>
#include "matahari/logging.h"
#include "matahari/sysconfig.h"
#include "matahari/utilities.h"


MH_TRACE_INIT_DATA(mh_sysconfig);

static int
download(const char *uri, FILE *fp)
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
    }
    curl_global_cleanup();
    return 0;
}


static int
mh_sysconfig_run_puppet(const char *uri, const char *data)
{
    gboolean ret;
    GError *error;
    char filename[PATH_MAX + 1];
    gchar cmd[PATH_MAX];
    int fd;
    FILE *fp;

    if (uri != NULL) {
		strncpy(filename, "puppet_conf_XXXXXX", sizeof(filename) -1);
		fd = mkstemp(filename);
		if (fd < 0) {
		    return -1;
		}
		fp = fdopen(fd, "w+b");
		if (fp == NULL) {
		    return -1;
		}
		if ((download(uri, fp)) != 0) {
		    fclose(fp);
		    return -1;
		}
		fclose(fp);
    } else if (data != NULL) {
		strncpy(filename,"puppet_conf_blob", sizeof(filename) -1);
		g_file_set_contents(filename, data, strlen(data), NULL);
    } else {
    	return -1;
    }

    g_snprintf(cmd, sizeof(cmd), "puppet %s", filename);
    ret = g_spawn_async(NULL, (gchar **)cmd, NULL, G_SPAWN_SEARCH_PATH,
            NULL, NULL, NULL, &error);
    fprintf(stderr, "bool: %d", ret);
    if (ret == FALSE) {
        g_error_free(error);
        return -1;
    }
    return 0;
}

static int
mh_sysconfig_run_augeas(const char *query, const char *data)
{
    mh_warn("not implemented\n");
    return 0;
}

static const char *
mh_sysconfig_query_augeas(const char *query)
{
    const char *data = NULL;
    mh_warn("not implemented\n");
    return data;
}

int
mh_sysconfig_run_uri(const char *uri, uint32_t flags, const char *scheme,
        const char *key)
{
	int rc = 0;

	if (strcasecmp(scheme, "puppet") == 0) {
        rc = mh_sysconfig_run_puppet(uri, NULL);
	} else {
        rc = -1;
	}
	return rc;
}

int
mh_sysconfig_run_string(const char *data, uint32_t flags, const char *scheme,
        const char *key)
{
	int rc = 0;

	if (strcasecmp(scheme, "puppet") == 0) {
        rc = mh_sysconfig_run_puppet(NULL, data);
	} else {
	    rc = -1;
	}
	return rc;
}

const char *
mh_sysconfig_query(const char *query, uint32_t flags, const char *scheme)
{
	const char *data = NULL;

	if (strcasecmp(scheme, "augeas") == 0) {
	    data = mh_sysconfig_query_augeas(query);
	}

	return data;
}
