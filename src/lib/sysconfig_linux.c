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

static conf_t cf = {};

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

int
mh_sysconfig_run_uri(const char *uri, int flags, const char *scheme) {
	int rc = 0;

	if (g_strcmp0(scheme, "puppet") == 0) {
        cf.uri = uri;
        cf.scheme = scheme;
        rc = mh_sysconfig_run_puppet(&cf);
	} else {
        rc = -1;
	}
	return rc;
}

int
mh_sysconfig_run_string(const char *data, int flags, const char *scheme) {
	int rc = 0;

	if (g_strcmp0(scheme, "puppet") == 0) {
        cf.data = data;
        cf.scheme = scheme;
        rc = mh_sysconfig_run_puppet(&cf);
	} else {
	    rc = -1;
	}
	return rc;
}

int
mh_sysconfig_query(const char *query, const char *data, int flags, const char *scheme) {
	int rc = 0;

	if (g_strcmp0(scheme, "augeas") == 0) {
		cf.query = query;
		cf.data = data;
		cf.scheme = scheme;
		rc = mh_sysconfig_run_augeas(&cf);
	} else {
		rc = -1;
	}
	return rc;
}


int
mh_sysconfig_run_puppet(conf_t *cf)
{
    gboolean ret;
    GError *error;
    char *filename = NULL;
    gchar cmd[PATH_MAX];
    int fd;
    FILE *fp;

    if (cf->uri != NULL) {
		filename = strdup("puppet_conf_XXXXXX");
		fd = mkstemp(filename);
		fp = fdopen(fd, "w+b");
		if ((download(cf->uri, fp)) != 0) {
		    fclose(fp);
		    return -1;
		}
		fclose(fp);
    } else if (cf->data != NULL) {
		filename = strdup("puppet_conf_blob");
		g_file_set_contents(filename, cf->data, strlen(cf->data), NULL);
    } else {
    	return -1;
    }

    g_snprintf(cmd, sizeof(cmd), "puppet %s", filename);
    fprintf(stderr, "%s", cmd);
    ret = g_spawn_async(NULL, (gchar **)cmd, NULL, G_SPAWN_SEARCH_PATH,
            NULL, NULL, NULL, &error);
    fprintf(stderr, "bool: %d", ret);
    if (ret == FALSE) {
        g_error_free(error);
        free(filename);
        return -1;
    }

    free(filename);

    return 0;
}

int
mh_sysconfig_run_augeas(conf_t *cf)
{
    fprintf(stderr, "not implemented\n");
    return 0;
}

int
mh_sysconfig_query_augeas(conf_t *cf)
{
    fprintf(stderr, "not implemented\n");
    return 0;
}
