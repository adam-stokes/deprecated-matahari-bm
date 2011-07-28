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
#ifndef WIN32
#include <sys/socket.h>
#include <netdb.h>
#endif
}

using namespace qpid::management;
using namespace qpid::client;
using namespace std;


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

static struct MatahariAgentImpl *
mh_qmf_connect(qpid::types::Variant::Map &urlMap, qpid::types::Variant::Map &options,
        struct MatahariAgentImpl *_impl)
{
    _impl->_amqp_connection = qpid::messaging::Connection(urlMap["uri"], options);
    try {
        _impl->_amqp_connection.open();
    } catch (const std::exception& err) {
        while (!_impl->_amqp_connection.isOpen()) {
            mh_info("Trying DNS SRV");
            _impl->_amqp_connection = qpid::messaging::Connection(urlMap["dnssrv"], options);
            try {
                _impl->_amqp_connection.open();
            } catch (const std::exception& err2) {
                mh_info("Trying qpid broker %s again", urlMap["servername"].asString().c_str());
                _impl->_amqp_connection = qpid::messaging::Connection(urlMap["uri"], options);
                try {
                    _impl->_amqp_connection.open();
                } catch (const std::exception& err3) {
                    g_usleep(G_USEC_PER_SEC);
                }
            }
        }
    }

    return _impl;
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
connection_option(int code, const char *name, const char *arg, void *userdata)
{
    qpid::types::Variant::Map *options = static_cast<qpid::types::Variant::Map*>(userdata);

    if(strcmp(name, "service") == 0) {
        options->insert(std::pair<std::string, qpid::types::Variant>("sasl-service", arg));
        options->insert(std::pair<std::string, qpid::types::Variant>("sasl-mechanism", "GSSAPI"));

    } else if(strcmp(name, "reconnect") == 0) {
        if(arg && strcmp(arg, "no") == 0) {
            options->insert(std::pair<std::string, qpid::types::Variant>("reconnect", false));
        }

    } else {
        options->insert(std::pair<std::string, qpid::types::Variant>(name, arg));
    }
    return 0;
}

int print_help(int code, const char *name, const char *arg, void *userdata)
{
    int lpc = 0;
    printf("Usage:\tmatahari-%sd <options>\n", (char *)userdata);
    printf("\nCommon options:\n");
    printf("\t-h | --help             print this help message.\n");
    printf("\t-b | --broker value     specify broker host name..\n");
    printf("\t-p | --port value       specify broker port.\n");
    printf("\t-u | --username value   username to use for authentication purproses.\n");
    printf("\t-P | --password value   password to use for authentication purproses.\n");
    printf("\t-s | --service value    service name to use for authentication purproses.\n");
    printf("\t-r | --reconnect value  attempt to reconnect on failure.\n");
#ifdef MH_SSL
    printf("\t-C | --ssl-cert-db             specify certificate database.\n");
    printf("\t-N | --ssl-cert-name           specify certificate name.\n");
    printf("\t-f | --ssl-cert-password-file  specify certificate password file.\n");
#endif

    printf("\nCustom options:\n");
    for(lpc = 0; lpc < DIMOF(matahari_options); lpc++) {
        if(matahari_options[lpc].callback
            && matahari_options[lpc].callback != connection_option) {
            printf("\t-%c | --%s\t %s\n", matahari_options[lpc].code,
                   matahari_options[lpc].long_name, matahari_options[lpc].description);
        }
    }
    return 0;
}

qpid::types::Variant::Map
mh_parse_options(const char *proc_name, int argc, char **argv, qpid::types::Variant::Map &options)
{
    std::stringstream url;
    qpid::types::Variant::Map urlMap;

    const char *ssl_cert_db = NULL;
    const char *ssl_cert_name = NULL;
    const char *ssl_cert_password_file = NULL;
    int lpc = 0;

    options["reconnect"] = false;

    /* Force local-only handling */
    mh_add_option('b', required_argument, "broker",                    NULL, NULL, NULL);
    mh_add_option('p', required_argument, "port",                    NULL, NULL, NULL);
#ifdef MH_SSL
    mh_add_option('N', required_argument, "ssl-cert-name",            NULL, NULL, NULL);
    mh_add_option('C', required_argument, "ssl-cert-db",            NULL, NULL, NULL);
    mh_add_option('f', required_argument, "ssl-cert-password-file", NULL, NULL, NULL);
#endif

    mh_add_option('u', required_argument, "username",  NULL, &options, connection_option);
    mh_add_option('P', required_argument, "password",  NULL, &options, connection_option);
    mh_add_option('s', required_argument, "service",   NULL, &options, connection_option);
    mh_add_option('r', required_argument, "reconnect", NULL, &options, connection_option);

#ifdef WIN32
    char *value = NULL;

    if (RegistryRead(HKEY_LOCAL_MACHINE,
                     L"SYSTEM\\CurrentControlSet\\services\\Matahari",
                     L"broker", &value) == 0) {
        urlMap["servername"] = value;
        value = NULL;
    }

    if (RegistryRead(HKEY_LOCAL_MACHINE,
                     L"SYSTEM\\CurrentControlSet\\services\\Matahari",
                     L"port", &value) == 0) {
        urlMap["serverport"] = atoi(value);
        free(value);
        value = NULL;
    }

    for (lpc = 0; lpc < DIMOF(matahari_options); lpc++) {
        if (matahari_options[lpc].callback) {
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
    struct addrinfo hints, *res;
    int rc, arg;
    char query[NS_MAXDNAME];
    char target[NS_MAXDNAME];


    /* Force more local-only processing */
    mh_add_option('h', no_argument, "help", NULL, NULL, NULL);
    mh_add_option('v', no_argument, "verbose", NULL, NULL, NULL);

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

    while ((arg = getopt_long(argc, argv, opt_string, long_opts, &idx)) != -1) {
        switch (arg) {
            case 'h':
                print_help('h', NULL, NULL, (void*)proc_name);
                exit(0);
                break;
            case 'v':
                mh_log_level++;
                mh_enable_stderr(1);
                break;
            case 'p':
                urlMap["serverport"] = optarg;
                break;
            case 'b':
                urlMap["servername"] = optarg;
#ifndef WIN32
                memset(&hints, 0, sizeof(struct addrinfo));
                hints.ai_family = AF_UNSPEC;
                if ((rc = getaddrinfo(urlMap["servername"].asString().c_str(), NULL, &hints, &res)) != 0) {
                    urlMap["servername"] = string("localhost");
                }
#endif
                break;
            case 'N':
                ssl_cert_name = optarg;
                break;
            case 'C':
                ssl_cert_db = optarg;
                setenv("QPID_SSL_CERT_DB", optarg, 1);
                if (!g_file_test(ssl_cert_db, G_FILE_TEST_IS_DIR)) {
                    fprintf(stderr, "SSL Certificate database is not accessible. See --help\n");
                    exit(1);
                }
                break;
            case 'f':
                ssl_cert_password_file = optarg;
                setenv("QPID_SSL_CERT_PASSWORD_FILE", optarg, 1);
                if (!g_file_test(ssl_cert_password_file, G_FILE_TEST_EXISTS)) {
                    fprintf(stderr, "SSL Password file is not accessible. See --help.\n");
                    exit(1);
                }
                break;
            default:
                if(arg > 0 && arg < DIMOF(matahari_options) && matahari_options[arg].callback) {
                    matahari_options[arg].callback(
                        matahari_options[arg].code, matahari_options[arg].long_name,
                        optarg, matahari_options[arg].userdata);

                } else {
                    print_help(arg, NULL, NULL, (void*)proc_name);
                    exit(1);
                }
                break;
        }
    }
    free(long_opts);
#endif

#ifdef MH_SSL
    if (ssl_cert_name && ssl_cert_db && ssl_cert_password_file) {
        qpid::sys::ssl::SslOptions ssl_options;
        ssl_options.certDbPath = strdup(ssl_cert_db);
        ssl_options.certName = strdup(ssl_cert_name);
        ssl_options.certPasswordFile = strdup(ssl_cert_password_file);
        qpid::sys::ssl::initNSS(ssl_options, true);

    } else if (ssl_cert_name || ssl_cert_db || ssl_cert_password_file) {
        fprintf(stderr, "To enable SSL, you must supply a cert name, db and password file. See --help.\n");
        exit(1);
    }
#endif

    if (ssl_cert_name && ssl_cert_db && ssl_cert_password_file) {
        urlMap["protocol"] = "ssl";
    } else {
        urlMap["protocol"] = "tcp";
    }

    if(urlMap["servername"].asString().empty()) {
        urlMap["servername"] = string("localhost");
    }
    if(urlMap["serverport"].asString().empty()) {
        urlMap["serverport"] = string("49000");
    }
    url << "amqp:" << urlMap["protocol"] << ":" << urlMap["servername"] << ":" << urlMap["serverport"] ;
    urlMap["uri"] = url.str();

    // Go ahead and verify if this is a DNS SRV record
    g_snprintf(query, sizeof(query), "_matahari._tcp.%s",
                        urlMap["servername"].asString().c_str());
    rc = mh_srv_lookup(query, target, sizeof(target));
    if (rc == 0) {
        std::stringstream dnsuri;
        url << target << ":" << urlMap["serverport"];
        urlMap["dnssrv"] = dnsuri.str();
    }

    return urlMap;
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

static int should_daemonize(int code, const char *name, const char *arg, void *userdata)
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
    qpid::types::Variant::Map options;
    int res = 0;

    /* Set up basic logging */
    mh_log_init(proc_name, LOG_INFO, TRUE);
    mh_add_option('d', no_argument, "daemon", "run as a daemon", NULL, should_daemonize);

    qpid::types::Variant::Map urlMap = mh_parse_options(proc_name, argc, argv, options);

    /* Re-initialize logging now that we've completed option processing */
    mh_log_init(proc_name, mh_log_level, mh_log_level > LOG_INFO);

    // Set up the cleanup handler for sigint
    signal(SIGINT, shutdown);

    mh_info("Connecting %s to Qpid broker at %s", proc_name, urlMap["uri"].asString().c_str());

    mh_qmf_connect(urlMap, options, _impl);
    _impl->_agent_session = qmf::AgentSession(_impl->_amqp_connection);
    _impl->_agent_session.setVendor("matahariproject.org");
    _impl->_agent_session.setProduct(proc_name);
    _impl->_agent_session.setAttribute("uuid", mh_uuid());
    _impl->_agent_session.setAttribute("hostname", mh_hostname());

    _impl->_agent_session.open();

    /* Do any setup required by our agent */
    if (this->setup(_impl->_agent_session) < 0) {
        mh_err("Failed to set up broker connection to %s for %s\n",
               urlMap["servername"].asString().c_str(), proc_name);
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
