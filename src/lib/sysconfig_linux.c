/* sysconfig.c - Copyright (C) 2011 Red Hat, Inc.
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
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, uri);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        res = curl_easy_perform(curl);

        if(CURLE_OK != res) {
            return -1;
        }
    }
    curl_global_cleanup();
    return 0;
}

void
mh_run_puppet(const char *data, int flags, int scheme)
{
    int rc;
    char *filename = NULL, *cmd = NULL;
    int fd;
    FILE *fp;
    
    switch(scheme) {
        case REMOTE:
            filename = strdup("puppet_conf_XXXXXX");
            fd = mkstemp(filename);
            fp = fdopen(fd, "w+b");
            rc = download(data, fp);
            fclose(fp);
            break;
        case LOCAL:
            filename = strdup(data);
            break;
        case BLOB:
            filename = strdup("puppet_conf_blob");
            g_file_set_contents(filename, data, strlen(data), NULL);
            break;
        default:
            break;
    }
    
    asprintf(cmd, "puppet %s", filename);
    rc = system(cmd);
    free(filename);
    free(cmd);
}

void
mh_run_augeas(const char *data, int flags, int scheme)
{
    fprintf(stderr, "not implemented\n");    
}

void
mh_query_augeas(const char *query, const char *data, int flags, int scheme)
{
    fprintf(stderr, "no implemeneted\n");
}