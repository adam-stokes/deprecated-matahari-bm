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
#include <getopt.h>
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
#include "matahari/mh_agent.h"

extern "C" {
#include "matahari/logging.h"
}

using namespace qpid::management;
using namespace qpid::client;
using namespace std;

void
shutdown(int /*signal*/)
{
    exit(0);
}

#ifdef WIN32
#define BUFFER_SIZE 1024
static void
RegistryRead(HKEY hHive, const wchar_t *szKeyPath, const wchar_t *szValue,
             char **out)
{
    HKEY hKey;
    DWORD nSize = BUFFER_SIZE;
    wchar_t szData[BUFFER_SIZE];
    long lSuccess = RegOpenKey(hHive, szKeyPath, &hKey);

    if (lSuccess != ERROR_SUCCESS) {
        mh_debug("Could not open %ls key from the registry: %ld", szKeyPath,
                 lSuccess);
        return;
    }

    lSuccess = RegQueryValueEx(hKey, szValue, NULL, NULL, (LPBYTE) szData,
                               &nSize);
    if (lSuccess != ERROR_SUCCESS) {
        mh_debug("Could not read '%ls[%ls]' from the registry: %ld", szKeyPath,
                 szValue, lSuccess);
        return;
    }
    mh_info("Obtained '%ls[%ls]' = '%ls' from the registry", szKeyPath, szValue,
            szData);
    if (out) {
        *out = (char *) malloc(BUFFER_SIZE);
        wcstombs(*out, szData, (size_t) BUFFER_SIZE);
    }
}
#else
struct option opt[] = {
    {"help", no_argument, NULL, 'h'},
    {"daemon", no_argument, NULL, 'd'},
    {"broker", required_argument, NULL, 'b'},
    {"gssapi", no_argument, NULL, 'g'},
    {"username", required_argument, NULL, 'u'},
    {"password", required_argument, NULL, 'P'},
    {"service", required_argument, NULL, 's'},
    {"port", required_argument, NULL, 'p'},
    {"reconnect", required_argument, NULL, 'r'},
#ifdef MH_SSL
    {"ssl-cert-name", required_argument, NULL, 'N'},
    {"ssl-cert-db", required_argument, NULL, 'C'},
    {"ssl-cert-password-file", required_argument, NULL, 'f'},
#endif
    {0, 0, 0, 0}
};

static void
print_usage(const char *proc_name)
{
    printf("Usage:\tmatahari-%sd <options>\n", proc_name);
    printf("\t-d | --daemon                  run as a daemon.\n");
    printf("\t-h | --help                    print this help message.\n");
    printf("\t-b | --broker                  specify broker host name..\n");
    printf("\t-g | --gssapi                  force GSSAPI authentication.\n");
    printf("\t-u | --username                username to use for authentication purproses.\n");
    printf("\t-P | --password                password to use for authentication purproses.\n");
    printf("\t-s | --service                 service name to use for authentication purproses.\n");
    printf("\t-p | --port                    specify broker port.\n");
    printf("\t-r | --reconnect [yes|no]      attempt to reconnect on failure.\n");
#ifdef MH_SSL
    printf("\t-N | --ssl-cert-name           specify certificate name.\n");
    printf("\t-C | --ssl-cert-db             specify certificate database.\n");
    printf("\t-f | --ssl-cert-password-file  specify certificate password file.\n");
#endif
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

int
MatahariAgent::init(int argc, char **argv, const char* proc_name)
{
#ifdef WIN32
    char *value = NULL;
#else
    int arg;
    int idx = 0;
    bool daemonize = false;
#endif

    bool gssapi = false;
    bool reconnect = true;
    char *protocol = NULL;
    char *servername = NULL;
    char *username  = NULL;
    char *password  = NULL;
    char *service   = NULL;
#ifdef MH_SSL
    char *ssl_cert_db = NULL;
    char *ssl_cert_name = NULL;
    char *ssl_cert_password_file = NULL;
#endif
    int serverport  = MATAHARI_PORT;
    int res = 0;

    /* Set up basic logging */
    mh_log_init(proc_name, LOG_INFO, FALSE);

#ifdef WIN32
    RegistryRead (
        HKEY_LOCAL_MACHINE,
        L"SYSTEM\\CurrentControlSet\\services\\Matahari",
        L"DebugLevel",
        &value);

    if (value) {
        mh_log_level = LOG_INFO + atoi(value);
        free(value);
        value = NULL;
    }

    RegistryRead (
        HKEY_LOCAL_MACHINE,
        L"SYSTEM\\CurrentControlSet\\services\\Matahari",
        L"Broker",
        &servername);

    RegistryRead (
        HKEY_LOCAL_MACHINE,
        L"SYSTEM\\CurrentControlSet\\services\\Matahari",
        L"Port",
        &value);

    if (value) {
        serverport = atoi(value);
        free(value);
        value = NULL;
    }

    RegistryRead (
        HKEY_LOCAL_MACHINE,
        L"SYSTEM\\CurrentControlSet\\services\\Matahari",
        L"Service",
        &service);

    RegistryRead (
        HKEY_LOCAL_MACHINE,
        L"SYSTEM\\CurrentControlSet\\services\\Matahari",
        L"User",
        &username);
    RegistryRead (
        HKEY_LOCAL_MACHINE,
        L"SYSTEM\\CurrentControlSet\\services\\Matahari",
        L"Password",
        &password);

#else

    // Get args
    while ((arg = getopt_long(argc, argv, "hdb:gr:u:P:s:p:vN:C:f:", opt, &idx)) != -1) {
        switch (arg) {
        case 'h':
        case '?':
            print_usage(proc_name);
            exit(0);
            break;
        case 'd':
            daemonize = true;
            break;
        case 'v':
            mh_log_level++;
            mh_enable_stderr(1);
            break;
        case 's':
            if (optarg) {
                service = strdup(optarg);
            } else {
                print_usage(proc_name);
                exit(1);
            }
            break;
        case 'u':
            if (optarg) {
                username = strdup(optarg);
            } else {
                print_usage(proc_name);
                exit(1);
            }
            break;
        case 'P':
            if (optarg) {
                password = strdup(optarg);
            } else {
                print_usage(proc_name);
                exit(1);
            }
            break;
        case 'g':
            gssapi = true;
            break;
        case 'r':
			if (optarg) {
				if (strcmp(optarg, "no") == 0) {
					reconnect = false;
				}
			} else {
				print_usage(proc_name);
				exit(1);
			}
            break;
        case 'p':
            if (optarg) {
                serverport = atoi(optarg);
            } else {
                print_usage(proc_name);
                exit(1);
            }
            break;
        case 'b':
            if (optarg) {
                servername = strdup(optarg);
            } else {
                print_usage(proc_name);
                exit(1);
            }
            break;
#ifdef MH_SSL
        case 'N':
            if (optarg) {
                protocol = strdup("ssl");
                ssl_cert_name = strdup(optarg);
            } else {
                fprintf(stderr, "An SSL Certificate name must be supplied.\n\n");
                print_usage(proc_name);
                exit(1);
            }
            break;
        case 'C':
            if (optarg) {
                setenv("QPID_SSL_CERT_DB", optarg, 1);
                ssl_cert_db = strdup(optarg);
            } else {
                print_usage(proc_name);
                exit(1);
            }
            break;
        case 'f':
            if (optarg) {
                setenv("QPID_SSL_CERT_PASSWORD_FILE", optarg, 1);
                ssl_cert_password_file = strdup(optarg);
            } else {
                print_usage(proc_name);
                exit(1);
            }
            break;
#endif
        default:
            fprintf(stderr, "unsupported option '-%c'.  See --help.\n", arg);
            print_usage(proc_name);
            exit(0);
            break;
        }
    }

    if (daemonize == true) {
        if (daemon(0, 0) < 0) {
            fprintf(stderr, "Error daemonizing: %s\n", strerror(errno));
            exit(1);
        }
    }
    
#ifdef MH_SSL
    if (ssl_cert_name && ssl_cert_db && ssl_cert_password_file) {
        if (!g_file_test(ssl_cert_password_file, G_FILE_TEST_EXISTS)) {
            fprintf(stderr, "SSL Password file is not accessible. See --help.\n");
            exit(1);
        }
        if (!g_file_test(ssl_cert_db, G_FILE_TEST_IS_DIR)) {
            fprintf(stderr, "SSL Certificate database is not accessible. See --help\n");
            exit(1);
        }

        qpid::sys::ssl::SslOptions ssl_options;
        ssl_options.certDbPath = strdup(ssl_cert_db);
        ssl_options.certName = strdup(ssl_cert_name);
        ssl_options.certPasswordFile = strdup(ssl_cert_password_file);
        qpid::sys::ssl::initNSS(ssl_options, true);
    }
#endif
    
#endif

    if (!servername || *servername == '\0') {
        servername = strdup(MATAHARI_BROKER);
    }
    
    if (!protocol || *protocol == '\0') {
        protocol = strdup("tcp");
    }

    /* Re-initialize logging now that we've completed option processing */
    mh_log_init(proc_name, mh_log_level, mh_log_level > LOG_INFO);

    // Set up the cleanup handler for sigint
    signal(SIGINT, shutdown);

    mh_info("Connecting to Qpid broker at %s on port %d", servername,
            serverport);

    // Create a v2 API options map.
    qpid::types::Variant::Map options;
    options["reconnect"] = reconnect;
    if (username && *username) {
        options["username"] = username;
    }
    if (password && *password) {
        options["password"] = password;
    }
    if (service && *service) {
        options["sasl-service"] = service;
    }
    if (gssapi) {
        options["sasl-mechanism"] = "GSSAPI";
    }

    std::stringstream url;
    url << "amqp:" << protocol << ":" << servername << ":" << serverport ;

    _amqp_connection = qpid::messaging::Connection(url.str(), options);
    _amqp_connection.open();

    _agent_session = qmf::AgentSession(_amqp_connection);
    _agent_session.setVendor("matahariproject.org");
    _agent_session.setProduct(proc_name);

    _agent_session.open();

    /* Do any setup required by our agent */
    if (this->setup(_agent_session) < 0) {
        mh_err("Failed to set up broker connection to %s on %d for %s\n",
               servername, serverport, proc_name);
        res = -1;
        goto return_cleanup;
    }

    this->mainloop = g_main_new(FALSE);
    this->qpid_source = mainloop_add_qmf(G_PRIORITY_HIGH, _agent_session,
                                         mh_qpid_callback, mh_qpid_disconnect,
                                         this);

return_cleanup:

    free(servername);
    free(username);
    free(password);
    free(service);
#ifdef MH_SSL
    free(ssl_cert_name);
    free(ssl_cert_db);
    free(ssl_cert_password_file);
#endif
    free(protocol);

    return res;
}

void
MatahariAgent::run()
{
    mh_trace("Starting agent mainloop");
    g_main_run(this->mainloop);
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
