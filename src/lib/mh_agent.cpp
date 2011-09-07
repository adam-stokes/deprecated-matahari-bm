/* mh_agent.cpp - Copyright (C) 2010 Red Hat, Inc.
 * Written by Andrew Beekhof <andrew@beekhof.net>
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

#ifdef WIN32
#include <windows.h>
int use_stderr = 1;
#else
int use_stderr = 0;
#endif

#include <iostream>
#include <fstream>
#include <string.h>
#include <sstream>
#include <errno.h>
#include <vector>
#include <exception>

#include <signal.h>
#include <stdlib.h>
#include <cstdlib>

#include <qpid/sys/Time.h>
#include <qpid/agent/ManagementAgent.h>
#include <qpid/client/ConnectionSettings.h>
// #include <qpid/sys/ssl/util.h>
#include <qmf/DataAddr.h>
#include "matahari/agent.h"

extern "C" {
#include <sys/types.h>
#include "matahari/logging.h"
#include "matahari/dnssrv.h"
#include "matahari/utilities.h"
#ifndef WIN32
#include <sys/socket.h>
#include <netdb.h>
#endif
}

using namespace qpid::management;
using namespace qpid::client;
using namespace std;

typedef qpid::types::Variant::Map OptionsMap;


struct MatahariAgentImpl {
    GMainLoop *_mainloop;
    mainloop_qmf_t *_qpid_source;

    qmf::AgentSession _agent_session;
    qpid::messaging::Connection _amqp_connection;
};


int print_help(int code, const char *name, const char *arg, void *userdata);

void
shutdown(int /*signal*/)
{
    exit(0);
}


#ifndef MAX_CHAR
/* 'z' + 1 */
#define MAX_CHAR 123
#endif

#ifdef WIN32
#define BUFFER_SIZE 1024
static int
RegistryRead(
    HKEY hHive, const wchar_t *szKeyPath, const wchar_t *szValue, char **out)
{
    HKEY hKey;
    DWORD nSize = BUFFER_SIZE;
    wchar_t szData[BUFFER_SIZE];
    long lSuccess = RegOpenKey(hHive, szKeyPath, &hKey);

    if (lSuccess != ERROR_SUCCESS) {
        mh_debug("Could not open %ls key from the registry: %ld",
                 szKeyPath, lSuccess);
        return -1;
    }

    lSuccess = RegQueryValueEx(hKey, szValue, NULL, NULL, (LPBYTE) szData,
                               &nSize);
    if (lSuccess != ERROR_SUCCESS) {
        mh_debug("Could not read '%ls[%ls]' from the registry: %ld",
                 szKeyPath, szValue, lSuccess);
        return -1;
    }
    mh_info("Obtained '%ls[%ls]' = '%ls' from the registry", szKeyPath, szValue,
            szData);
    if (out) {
        *out = (char *) malloc(BUFFER_SIZE);
        wcstombs(*out, szData, (size_t) BUFFER_SIZE);
    }
    return 0;
}
#endif

static gboolean
mh_qpid_callback(qmf::AgentSession session, qmf::AgentEvent event,
                 gpointer user_data)
{
    MatahariAgent *agent = (MatahariAgent*) user_data;
    mh_trace("Qpid message recieved");
    if (event.hasDataAddr()) {
        mh_trace("Message is for %s (type: %s)",
                 event.getDataAddr().getName().c_str(),
                 event.getDataAddr().getAgentName().c_str());
    }
    return agent->invoke(session, event, user_data);
}

static void
mh_qpid_disconnect(gpointer user_data)
{
    mh_err("Qpid connection closed");
}


typedef struct mh_opt_s
{
        int            code;
        int            has_arg;
        const char *long_name;
        const char *description;
        void           *userdata;
        int          (*callback)(int code, const char *name, const char *arg, void *userdata);

} mh_option;


static mh_option matahari_options[MAX_CHAR];

static int
map_option(int code, const char *name, const char *arg, void *userdata)
{
    OptionsMap *options = (OptionsMap*)userdata;

    if(strcmp(name, "verbose") == 0) {
        mh_log_level++;
        mh_enable_stderr(1);

    } else if(strcmp(name, "broker") == 0) {
        (*options)["servername"] = arg;

    } else if(strcmp(name, "port") == 0) {
        (*options)["serverport"] = atoi(arg);

    } else if(strcmp(name, "dns-srv") == 0) {
        (*options)["dns-srv"] = 1;

    } else {
        (*options)[name] = arg;
    }
    return 0;
}

#ifdef MH_SSL
static int
ssl_option(int code, const char *name, const char *arg, void *userdata)
{
    qpid::sys::ssl::SslOptions *options = static_cast<qpid::sys::ssl::SslOptions*>(userdata);

    if(strcmp(name, "ssl-cert-db") == 0) {
        options->certDbPath = strdup(arg);
        setenv("QPID_SSL_CERT_DB", arg, 1);
        if (!g_file_test(arg, G_FILE_TEST_IS_DIR)) {
            fprintf(stderr, "SSL Certificate database is not accessible. See --help\n");
            exit(1);
        }

    } else if(strcmp(name, "ssl-cert-name") == 0) {
        options->certName = strdup(arg);

    } else if(strcmp(name, "ssl-cert-password-file") == 0) {
        options->certPasswordFile = strdup(arg);
        setenv("QPID_SSL_CERT_PASSWORD_FILE", arg, 1);
        if (!g_file_test(arg, G_FILE_TEST_EXISTS)) {
            fprintf(stderr, "SSL Password file is not accessible. See --help.\n");
            exit(1);
        }
    }
    return 0;
}
#endif /* MH_SSL */

static int
connection_option(int code, const char *name, const char *arg, void *userdata)
{
    OptionsMap *options = (OptionsMap*)userdata;

    if(strcmp(name, "service") == 0) {
        (*options)["sasl-service"] = arg;
        (*options)["sasl-mechanism"] = "GSSAPI";
    } else if(strcmp(name, "reconnect") == 0) {
        bool reconnect = !arg || (strcasecmp(arg, "no") != 0 &&
                                  strcasecmp(arg, "false") != 0);
        (*options)["reconnect"] = reconnect;
    } else {
        (*options)[name] = arg;
    }
    return 0;
}

int print_help(int code, const char *name, const char *arg, void *userdata)
{
    int lpc = 0;
    printf("Usage:\tmatahari-%sd <options>\n", (char *)userdata);

    printf("\nOptions:\n");
    printf("\t-h | --help             print this help message.\n");
    for(lpc = 0; lpc < DIMOF(matahari_options); lpc++) {
        if(matahari_options[lpc].callback
            && matahari_options[lpc].callback != connection_option) {
            printf("\t-%c | --%-10s\t%s\n", matahari_options[lpc].code,
                   matahari_options[lpc].long_name, matahari_options[lpc].description);
        }
    }
    return 0;
}

qpid::messaging::Connection
mh_connect(OptionsMap mh_options, OptionsMap amqp_options, int retry)
{
    int retries = 0;
    int backoff = 0;
    GList *srv_records = NULL, *cur_srv_record = NULL;
    struct mh_dnssrv_record *record;

    if (!mh_options.count("servername") || mh_options.count("dns-srv")) {
        /*
         * Do an SRV lookup either because no broker was specified at all,
         * or because a broker domain was given and the --srv option was
         * used specifically requesting an SRV lookup.
         */

        std::stringstream query;

        if (mh_options.count("servername")) {
            query << "_matahari._tcp." << mh_options["servername"];
        } else {
            query << "_matahari._tcp." << mh_dnsdomainname();
        }

        if ((cur_srv_record = srv_records = mh_dnssrv_lookup(query.str().c_str()))) {
            mh_info("SRV query successful: %s", query.str().c_str());
        } else {
            mh_info("SRV query not successful: %s", query.str().c_str());
        }
    }

    while (true) {
        std::stringstream url;

        if (srv_records) {
            /* Use the result of a DNS SRV lookup. */

            record = (struct mh_dnssrv_record *) cur_srv_record->data;

            url << "amqp:" << mh_options["protocol"];
            url << ":" << mh_dnssrv_record_get_host(record);
            url << ":" << mh_dnssrv_record_get_port(record);

            cur_srv_record = cur_srv_record->next;
            if (!cur_srv_record) {
                cur_srv_record = srv_records;
            }
        } else if (mh_options.count("servername")) {
            /* Use the explicitly specified broker hostname or IP address. */
            url << "amqp:" << mh_options["protocol"] << ":" << mh_options["servername"] << ":" << mh_options["serverport"] ;
        } else {
            /* If nothing else, try localhost */
            url << "amqp:" << mh_options["protocol"] << ":localhost:" << mh_options["serverport"] ;
        }

        retries++;
        qpid::messaging::Connection amqp = qpid::messaging::Connection(url.str(), amqp_options);
        if(retries < 5) {
            mh_info("Trying: %s", url.str().c_str());
        } else if(retries == 5) {
            mh_warn("Cannot find a QMF broker - will keep retrying silently");
        } else {
            backoff = retries % 300;
        }

        try {
            amqp.open();
            g_list_free_full(srv_records, mh_dnssrv_record_free);
            return amqp;

        } catch (const std::exception& err) {
            if(!retry) {
                goto bail;

            } else if(backoff) {
                g_usleep(backoff * G_USEC_PER_SEC);
            }
        }
    }
  bail:
    g_list_free_full(srv_records, mh_dnssrv_record_free);
    return NULL;
}

#ifndef WIN32
static void
read_environment(OptionsMap& options)
{
    const char *data;

    data = getenv("MATAHARI_BROKER");
    if (data && strlen(data)) {
        options["servername"] = data;
    }

    data = getenv("MATAHARI_PORT");
    if (data && strlen(data)) {
        options["serverport"] = data;
    }
}

static void
verify_broker_option(OptionsMap& options)
{
    if (!options.count("servername")) {
        return;
    }

    struct addrinfo hints, *res;
    std::string broker = options["servername"];

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;

    if (getaddrinfo(broker.c_str(), NULL, &hints, &res) == 0) {
        freeaddrinfo(res);
    } else {
        OptionsMap::iterator iter = options.find("servername");
        options.erase(iter);
        mh_err("Broker '%s' is not resolvable - ignoring", broker.c_str());
    }
}
#endif

OptionsMap
mh_parse_options(const char *proc_name, int argc, char **argv, OptionsMap &options)
{
    std::stringstream url;
    OptionsMap amqp_options;

    int lpc = 0;

    options["protocol"] = "tcp";
    options["serverport"] = "49000";
    amqp_options["reconnect"] = true;

    /* Force local-only handling */
    mh_add_option('b', required_argument, "broker",                 "specify broker host name", &options, map_option);
    mh_add_option('D', no_argument,       "dns-srv",                "interpret the value of --broker as a domain name for DNS SRV lookups", &options, map_option);
    mh_add_option('p', required_argument, "port",                   "specify broker ", &options, map_option);

    mh_add_option('u', required_argument, "username",  "username to use for authentication to the broker", &amqp_options, connection_option);
    mh_add_option('P', required_argument, "password",  "username to use for authentication to the broker", &amqp_options, connection_option);
    mh_add_option('s', required_argument, "service",   "service name to use for authentication to the broker", &amqp_options, connection_option);
    mh_add_option('r', required_argument, "reconnect", "attempt to reconnect if the broker connection is lost", &amqp_options, connection_option);

#ifdef MH_SSL
    qpid::sys::ssl::SslOptions ssl_options;
    mh_add_option('n', required_argument, "ssl-cert-name",          "name of the certificate to use", &ssl_options, ssl_option);
    mh_add_option('C', required_argument, "ssl-cert-db",            "file containing the certificate database", &ssl_options, ssl_option);
    mh_add_option('f', required_argument, "ssl-cert-password-file", "file containing the certificate password", &ssl_options, ssl_option);
#endif

#ifdef WIN32
    for (lpc = 0; lpc < DIMOF(matahari_options); lpc++) {
        if (matahari_options[lpc].callback) {
            char *value = NULL;
            wchar_t *name_ws = char2wide(matahari_options[lpc].long_name);
            if (RegistryRead (HKEY_LOCAL_MACHINE,
                             L"SYSTEM\\CurrentControlSet\\services\\Matahari",
                             name_ws, &value) == 0) {
                matahari_options[lpc].callback(
                    matahari_options[lpc].code, matahari_options[lpc].long_name,
                    value, matahari_options[lpc].userdata);
                free(value);
                value = NULL;
            }
            free(name_ws);
        }
    }

#else
    int idx = 0;
    int num_options = 0;
    int opt_string_len = 0;
    char opt_string[2 * DIMOF(matahari_options)];
    struct option *long_opts = (struct option *)calloc(1, sizeof(struct option));
    int arg;

    /* Force more local-only processing specific to linux */
    mh_add_option('h', no_argument, "help", NULL, NULL, NULL);
    mh_add_option('v', no_argument, "verbose", "Increase the log level", NULL, map_option);

    opt_string[0] = 0;
    for(lpc = 0; lpc < DIMOF(matahari_options); lpc++) {
        if(matahari_options[lpc].code) {
            long_opts = (struct option *)realloc(long_opts, (2 + num_options) * sizeof(struct option));
            long_opts[num_options].name = matahari_options[lpc].long_name;
            long_opts[num_options].has_arg = matahari_options[lpc].has_arg;
            long_opts[num_options].flag = NULL;
            long_opts[num_options].val = matahari_options[lpc].code;

            num_options++;

            long_opts[num_options].name = 0;
            long_opts[num_options].has_arg = 0;
            long_opts[num_options].flag = 0;
            long_opts[num_options].val = 0;

            opt_string[opt_string_len++] = matahari_options[lpc].code;
            if(matahari_options[lpc].has_arg == required_argument) {
                opt_string[opt_string_len++] = ':';
            }
            opt_string[opt_string_len] = 0;
        }
    }

    read_environment(options);

    while ((arg = getopt_long(argc, argv, opt_string, long_opts, &idx)) != -1) {
        if(arg == 'h') {
            print_help('h', NULL, NULL, (void*)proc_name);
            exit(0);

        } else if(arg > 0 && arg < DIMOF(matahari_options) && matahari_options[arg].callback) {
            matahari_options[arg].callback(
                matahari_options[arg].code, matahari_options[arg].long_name,
                optarg, matahari_options[arg].userdata);

        } else {
            print_help(arg, NULL, NULL, (void*)proc_name);
            exit(1);
        }
    }
    free(long_opts);

    if (!options.count("dns-srv")) {
        verify_broker_option(options);
    }
#endif

#ifdef MH_SSL
    if (ssl_options.certDbPath && ssl_options.certName && ssl_options.certPasswordFile) {
        options["protocol"] = "ssl";
        qpid::sys::ssl::initNSS(ssl_options, true);

    } else if (ssl_options.certDbPath || ssl_options.certName || ssl_options.certPasswordFile) {
        fprintf(stderr, "To enable SSL, you must supply a cert name, db and password file. See --help.\n");
        exit(1);
    }
#endif

    return amqp_options;
}

int
mh_add_option(int code, int has_arg, const char *name, const char *description,
              void *userdata, int(*callback)(int code, const char *name, const char *arg, void *userdata))
{
    if(code > 0 && code < DIMOF(matahari_options)) {
        if(matahari_options[code].code != 0) {
            mh_err("Replacing '-%c|--%s' with '-%c|--%s'",
                   matahari_options[code].code, matahari_options[code].long_name, code, name);
        }
        matahari_options[code].code = code;
        matahari_options[code].has_arg = has_arg;
        matahari_options[code].long_name = name;
        matahari_options[code].description = description;
        matahari_options[code].userdata = userdata;
        matahari_options[code].callback = callback;
        return 0;
    }
    return -1;
}

int
mh_should_daemonize(int code, const char *name, const char *arg, void *userdata)
{
#ifndef WIN32
    if (daemon(0, 0) < 0) {
        fprintf(stderr, "Error daemonizing: %s\n", strerror(errno));
        exit(1);
    }
#endif
    return 0;
}

MatahariAgent::MatahariAgent(): _impl(new MatahariAgentImpl())
{

}

MatahariAgent::~MatahariAgent()
{
    delete _impl;
}

qmf::AgentSession& MatahariAgent::getSession(void)
{
    return _impl->_agent_session;
}

int
MatahariAgent::init(int argc, char **argv, const char* proc_name)
{
    OptionsMap options;
    int res = 0;
    std::stringstream logname;
    logname << "matahari-" << proc_name;

    /* Set up basic logging */
    mh_log_init(proc_name, mh_log_level, FALSE);
    mh_add_option('d', no_argument, "daemon", "run as a daemon", NULL, mh_should_daemonize);

    OptionsMap amqp_options = mh_parse_options(proc_name, argc, argv, options);


    /* Re-initialize logging now that we've completed option processing */
    mh_log_init(strdup(logname.str().c_str()), mh_log_level, mh_log_level > LOG_INFO);

    // Set up the cleanup handler for sigint
    signal(SIGINT, shutdown);

    _impl->_amqp_connection = mh_connect(options, amqp_options, TRUE);

    _impl->_agent_session = qmf::AgentSession(_impl->_amqp_connection);
    _impl->_agent_session.setVendor("matahariproject.org");
    _impl->_agent_session.setProduct(proc_name);
    _impl->_agent_session.setAttribute("uuid", mh_uuid());
    _impl->_agent_session.setAttribute("hostname", mh_hostname());

    _impl->_agent_session.open();

    /* Do any setup required by our agent */
    if (this->setup(_impl->_agent_session) < 0) {
        mh_err("Failed to set up broker connection to %s for %s\n",
               options["servername"].asString().c_str(), proc_name);
        res = -1;
        goto return_cleanup;
    }

    _impl->_mainloop = g_main_new(FALSE);
    _impl->_qpid_source = mainloop_add_qmf(G_PRIORITY_HIGH, _impl->_agent_session,
                                           mh_qpid_callback, mh_qpid_disconnect,
                                           this);

return_cleanup:
    return res;
}

void
MatahariAgent::run()
{
    mh_trace("Starting agent mainloop");
    g_main_run(_impl->_mainloop);
}

static gboolean
mainloop_qmf_prepare(GSource* source, gint *timeout)
{
    mainloop_qmf_t *qmf = (mainloop_qmf_t *) source;
    if (qmf->event) {
        return TRUE;
    }

    *timeout = 1;
    return FALSE;
}

static gboolean
mainloop_qmf_check(GSource* source)
{
    mainloop_qmf_t *qmf = (mainloop_qmf_t *) source;
    if (qmf->event) {
        return TRUE;

    } else if (qmf->session.nextEvent(qmf->event,
                                      qpid::messaging::Duration::IMMEDIATE)) {
        return TRUE;
    }
    return FALSE;
}

static gboolean
mainloop_qmf_dispatch(GSource *source, GSourceFunc callback, gpointer userdata)
{
    mainloop_qmf_t *qmf = (mainloop_qmf_t *) source;
    mh_trace("%p", source);
    if (qmf->dispatch != NULL) {
        qmf::AgentEvent event = qmf->event;
        qmf->event = NULL;

        if (qmf->dispatch(qmf->session, event, qmf->user_data) == FALSE) {
            g_source_unref(source); /* Really? */
            return FALSE;
        }
    }

    return TRUE;
}

static void
mainloop_qmf_destroy(GSource *source)
{
    mainloop_qmf_t *qmf = (mainloop_qmf_t *) source;
    mh_trace("%p", source);

    if (qmf->dnotify) {
        qmf->dnotify(qmf->user_data);
    }
}

static GSourceFuncs mainloop_qmf_funcs = {
    mainloop_qmf_prepare,
    mainloop_qmf_check,
    mainloop_qmf_dispatch,
    mainloop_qmf_destroy,
};

mainloop_qmf_t *
mainloop_add_qmf(int priority, qmf::AgentSession session,
                 gboolean (*dispatch)(qmf::AgentSession session,
                                      qmf::AgentEvent event, gpointer userdata),
                 GDestroyNotify notify, gpointer userdata)
{
    GSource *source = NULL;
    mainloop_qmf_t *qmf_source = NULL;
    MH_ASSERT(sizeof(mainloop_qmf_t) > sizeof(GSource));
    source = g_source_new(&mainloop_qmf_funcs, sizeof(mainloop_qmf_t));
    MH_ASSERT(source != NULL);

    qmf_source = (mainloop_qmf_t *) source;
    qmf_source->id = 0;
    qmf_source->event = NULL;
    qmf_source->session = session;

    /*
     * Normally we'd use g_source_set_callback() to specify the dispatch
     * function, but we want to supply the qmf session too, so we store it in
     * qmf_source->dispatch instead
     */
    qmf_source->dnotify = notify;
    qmf_source->dispatch = dispatch;
    qmf_source->user_data = userdata;

    g_source_set_priority(source, priority);
    g_source_set_can_recurse(source, FALSE);

    qmf_source->id = g_source_attach(source, NULL);
    mh_info("Added source: %d", qmf_source->id);
    return qmf_source;
}

gboolean
mainloop_destroy_qmf(mainloop_qmf_t *source)
{
    g_source_remove(source->id);
    source->id = 0;
    g_source_unref((GSource *) source);

    return TRUE;
}
