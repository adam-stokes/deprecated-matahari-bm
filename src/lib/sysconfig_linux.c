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
#include <unistd.h>
#include <glib.h>
#include <curl/curl.h>
#include "matahari/logging.h"
#include "matahari/sysconfig.h"
#include "matahari/utilities.h"


MH_TRACE_INIT_DATA(mh_sysconfig);

struct OutFile {
    const char *filename;
    FILE *stream;
};

static size_t local_fwrite(void *buffer, size_t size,
                           size_t nmemb, void *stream) {
    struct OutFile *out = (struct OutFile *)stream;
    if(out && !out->stream) {
        out->stream = fopen(out->filename, "wb");
        if(!out->stream) {
            return -1;
        }
    }
    return fwrite(buffer, size, nmemb, out->stream);
}

static int puppet_run(const char *file)
{
    char *cmd;
    int ret;
    
    asprintf(cmd, "puppet %s", file);
    ret = system(cmd);
    
    return ret;
}

static int open_fd_out() {
    char fname[PATH_MAX] = "/tmp/matahari.sysconfig.XXXXXX";
    return mkstemp(fname);
}

void mh_configure_uri(const char *uri, int type)
{
    CURL *curl;
    CURLcode res;
    struct OutFile outfile={
        basename(uri),
        NULL
    };

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, uri);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, local_fwrite);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &outfile);
        res = curl_easy_perform(curl);

        if(CURLE_OK != res) {
            fprintf(stderr, "Failed %d\n", res);
        }
    }
    if(outfile.stream) {
        fclose(outfile.stream);
    }
    curl_global_cleanup();
}

void mh_configure_blob(const char *blob, int type)
{
    int fd = open_fd_out();
    size_t count;
    
    count = fwrite(blob, 1, strlen(blob), fd);
    fclose(fd);
}
