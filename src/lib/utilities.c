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

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include <stdio.h>
#include <errno.h>
#include <glib.h>
#include <sigar.h>

#include "matahari/logging.h"
#include "matahari/utilities.h"
#include "utilities_private.h"

MH_TRACE_INIT_DATA(mh_core);
int mh_log_level = LOG_NOTICE;
gboolean mh_stderr_enabled = FALSE;

#if __linux__
#  include <link.h>
#  include <unistd.h>
#  include <sys/wait.h>
#  include <sys/utsname.h>
#else
#  include <winsock.h>
#  include <windows.h>
#  include <winbase.h>
#endif

#if SUPPORT_TRACING
static int
mh_update_trace_data(struct _mh_ddebug_query *query, struct _mh_ddebug *start,
                     struct _mh_ddebug *stop)
{
    int lpc = 0;
    unsigned nfound = 0;
    struct _mh_ddebug *dp;
    const char *match = "unknown";

    MH_ASSERT(stop != NULL);
    MH_ASSERT(start != NULL);

    /* fprintf(stderr, "checking for fns: %s files: %s fmts: %s\n", */
    /*         query->functions, query->files, query->formats); */

    for (dp = start; dp != stop; dp++) {
        gboolean bump = FALSE;
        lpc++;
        /* fprintf(stderr, "checking: %-12s %20s:%u fmt:%s\n", */
        /*         dp->function, dp->filename, dp->lineno, dp->format); */

        if (query->functions &&
            strstr(query->functions, dp->function) != NULL) {
            match = "function";
            bump = TRUE;
        }

        if (query->files) {
            char token[500];
            const char *offset = NULL;
            const char *next = query->files;

            do {
                offset = next;
                next = strchrnul(offset, ',');
                snprintf(token, 499, "%.*s", (int)(next - offset), offset);

                if (query->files && strstr(dp->filename, token) != NULL) {
                    match = "file";
                    bump = TRUE;

                } else if (next[0] != 0) {
                    next++;
                }

            } while (bump == FALSE && next != NULL && next[0] != 0);
        }

        if (query->formats && strstr(query->formats, dp->format) != NULL) {
            match = "format";
            bump = TRUE;
        }

        if (bump) {
            nfound++;
            dp->bump = LOG_NOTICE;
            mh_log_always(LOG_INFO, "Detected '%s' match: %-12s %20s:%u fmt:%s",
                          match, dp->function, dp->filename, dp->lineno,
                          dp->format);
        }
    }

    query->total += lpc;
    query->matches += nfound;
    return nfound;
}

static int
mh_ddebug_callback(struct dl_phdr_info *info, size_t size, void *data)
{
    if (strlen(info->dlpi_name) > 0) {
        struct _mh_ddebug_query *query = data;

        void *handle;
        void *start;
        void *stop;
        char *error;

        handle = dlopen (info->dlpi_name, RTLD_LAZY);
        error = dlerror();
        if (!handle || error) {
            mh_err("%s", error);
            if (handle) {
                dlclose(handle);
            }
            return 0;
        }

        start = dlsym(handle, "__start___verbose");
        error = dlerror();
        if (error) {
            goto done;
        }

        stop = dlsym(handle, "__stop___verbose");
        error = dlerror();
        if (error) {
            goto done;

        } else {
            unsigned long int len = (unsigned long int) stop -
                                    (unsigned long int) start;
            mh_info("Checking for query matches in %lu trace symbols from: "
                    "%s (offset: %p)", len / sizeof(struct _mh_ddebug),
                    info->dlpi_name, start);

            mh_update_trace_data(query, start, stop);
        }
    done:
        dlclose(handle);
    }

    return 0;
}
#endif

static void
mh_glib_handler(const gchar *log_domain, GLogLevelFlags flags,
                const gchar *message, gpointer user_data)
{
    int log_level = LOG_WARNING;
    GLogLevelFlags msg_level = (flags & G_LOG_LEVEL_MASK);

    switch (msg_level) {
    case G_LOG_LEVEL_CRITICAL:
        /* log and record how we got here */
        mh_abort(__FILE__, __PRETTY_FUNCTION__, __LINE__, message, TRUE, TRUE);
        return;

    case G_LOG_FLAG_FATAL:    log_level = LOG_CRIT;   break;
    case G_LOG_LEVEL_ERROR:   log_level = LOG_ERR;    break;
    case G_LOG_LEVEL_MESSAGE: log_level = LOG_NOTICE; break;
    case G_LOG_LEVEL_INFO:    log_level = LOG_INFO;   break;
    case G_LOG_LEVEL_DEBUG:   log_level = LOG_DEBUG;  break;

    case G_LOG_LEVEL_WARNING:
    case G_LOG_FLAG_RECURSION:
    case G_LOG_LEVEL_MASK:
        log_level = LOG_WARNING;
        break;
    }

    mh_log(log_level, "%s: %s", log_domain, message);
}

void
mh_enable_stderr(gboolean to_stderr)
{
    mh_stderr_enabled = to_stderr;
}

void
mh_log_init(const char *ident, int level, gboolean to_stderr)
{
#if SUPPORT_TRACING
    gboolean search = FALSE;
    const char *env_value = NULL;
    struct _mh_ddebug_query query;
#endif

    mh_log_level = level;
    mh_stderr_enabled = to_stderr;

#ifdef __linux__
    openlog(ident, LOG_NDELAY | LOG_PID, LOG_DAEMON);
#endif

    /* Consistant glib logging */
    g_log_set_default_handler(mh_glib_handler, NULL);

    /* and for good measure... - this enum is a bit field (!) */
    g_log_set_always_fatal((GLogLevelFlags) 0); /*value out of range*/

#if SUPPORT_TRACING
    memset(&query, 0, sizeof(struct _mh_ddebug_query));

    env_value = getenv("MH_trace_files");
    if (env_value) {
        search = TRUE;
        query.files = env_value;
    }

    env_value = getenv("MH_trace_formats");
    if (env_value) {
        search = TRUE;
        query.formats = env_value;
    }

    env_value = getenv("MH_trace_functions");
    if (env_value) {
        search = TRUE;
        query.functions = env_value;
    }

    if (search) {
        mh_update_trace_data(&query, __start___verbose, __stop___verbose);
        dl_iterate_phdr(mh_ddebug_callback, &query);
        if (query.matches == 0) {
            mh_log_always(LOG_DEBUG, "no matches for query: {fn='%s', "
                          "file='%s', fmt='%s'} in %llu entries",
                          query.functions ? query.functions : "N/A",
                          query.files ? query.files : "N/A",
                          query.formats ? query.formats : "N/A",
                          query.total);
        } else {
            mh_log_always(LOG_INFO, "%llu matches for query: {fn='%s', "
                          "file='%s', fmt='%s'} in %llu entries",
                          query.matches,
                          query.functions ? query.functions : "N/A",
                          query.files ? query.files : "N/A",
                          query.formats ? query.formats : "N/A",
                          query.total);
        }
    }
#endif
}

void
mh_abort(const char *file, const char *function, int line,
         const char *assert_condition, int do_core, int do_fork)
{
#ifdef __linux__
    int rc = 0;
    int pid = 0;
    int status = 0;

    if (do_core == 0) {
        mh_err("%s: Triggered assert at %s:%d : %s",
               function, file, line, assert_condition);
        return;

    } else if (do_fork) {
        pid = fork();

    } else {
        mh_err("%s: Triggered fatal assert at %s:%d : %s",
               function, file, line, assert_condition);
    }

    switch (pid) {
    case -1:
        mh_crit("%s: Cannot create core for non-fatal assert at %s:%d : %s",
                function, file, line, assert_condition);
        return;

    default: /* Parent */
        mh_err("%s: Forked child %d to record non-fatal assert at %s:%d : %s",
               function, pid, file, line, assert_condition);
        do {
            rc = waitpid(pid, &status, 0);
            if (rc < 0 && errno != EINTR) {
                mh_perror(LOG_ERR, "%s: Cannot wait on forked child %d",
                          function, pid);
            }

        } while (rc < 0 && errno == EINTR);

        return;

    case 0: /* Child */
        abort();
        break;
    }
#else
    mh_err("%s: Triggered assert at %s:%d : %s", function, file, line,
           assert_condition);
    abort();
#endif
}

void
mh_log_fn(int priority, const char * fmt, ...)
{
    va_list ap;

    if (mh_stderr_enabled) {
        va_start(ap, fmt);
        vfprintf(stderr, fmt, ap);
#ifdef __linux__
        fprintf(stderr, "\n");
#else
        fprintf(stderr, "\n\r");
#endif
        va_end(ap);

#ifdef __linux__
    } else {
        va_start(ap, fmt);
        vsyslog(priority, fmt, ap);
        va_end(ap);
#endif
    }

    return;
}

const char *
mh_domainname(void)
{
    static char *domainname = NULL;

    if (domainname == NULL) {
        sigar_t *sigar;
        sigar_net_info_t netinfo;
        sigar_open(&sigar);
        sigar_net_info_get(sigar, &netinfo);
        domainname = strdup(netinfo.domain_name);
        sigar_close(sigar);
    }

    if (domainname == NULL) {
        return "";
    } else if (strlen(domainname) == 0 || strcasecmp(domainname, "(none)") == 0) {
        domainname = NULL;
        return "";
    }

    mh_trace("Got domainname: %s", domainname);
    return domainname;
}

const char *
mh_dnsdomainname(void)
{
    const char *res;

    res = mh_os_dnsdomainname();

    mh_trace("Got dnsdomainname: '%s'", res);

    return res;
}

const char *
mh_uuid(void)
{
    return mh_os_uuid();
}

const char *
mh_hostname(void)
{
    static char *hostname = NULL;

    if (hostname == NULL) {
        sigar_t *sigar;
        sigar_net_info_t netinfo;
        sigar_open(&sigar);
        sigar_net_info_get(sigar, &netinfo);
        hostname = strdup(netinfo.host_name);
        sigar_close(sigar);
    }

    if (hostname != NULL && strcmp(hostname, "localhost") == 0) {
        free(hostname);
        hostname = strdup(mh_uuid());
    }

    mh_trace("Got hostname: %s", hostname);
    return hostname;
}

char *mh_file_first_line(const char *file)
{
    GError* error = NULL;
    char *buffer = NULL;

    if(!g_file_test(file, G_FILE_TEST_EXISTS)) {
        return NULL;
    }

    if(g_file_get_contents(file, &buffer, NULL, &error) == FALSE) {
        buffer = strdup(error->message);
        g_error_free(error);
    }

    if (buffer) {
        /* Truncate after at most one line */
#ifdef __linux__
        char *tmp = strchrnul(buffer, '\n');
        if(tmp) {
            *tmp = '\0';
        }
#else
        int len = 0, lpc = 0;
        for(len = strlen(buffer); lpc < len; lpc++) {
            if(buffer[lpc] == '\n') {
                buffer[lpc] = '\0';
            }
        }
#endif
    }
    return buffer;
}

#ifdef WIN32
wchar_t *
char2wide(const char *str)
{
    wchar_t *result = NULL;
    if (str != NULL) {
        size_t str_size = strlen(str) + 1;
        result = malloc(str_size * sizeof(wchar_t));
        mbstowcs(result, str, str_size);
    }
    return result;
}
#endif


#ifndef HAVE_ASPRINTF
int
asprintf(char **ret, const char *fmt, ...)
{
    va_list ap;
    int len;
    char buf[1];

    /*
     * Ask vsnprintf() how many characters it would write if the buffer
     * were big enough.
     */
    va_start(ap, fmt);
    len = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    /*
     * Now go allocate a buffer that is big enough.
     */
    *ret = malloc(len + 1);

    /*
     * Now vsnprintf() the string into the final result buffer.
     */
    va_start(ap, fmt);
    vsnprintf(*ret, len + 1, fmt, ap);
    va_end(ap);

    return len;
}
#endif /* HAVE_ASPRINTF */

#ifndef HAVE_G_LIST_FREE_FULL
void
g_list_free_full (GList *list, GDestroyNotify free_func)
{
    g_list_foreach (list, (GFunc) free_func, NULL);
    g_list_free (list);
}
#endif /* HAVE_G_LIST_FREE_FULL */
