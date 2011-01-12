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
#include <errno.h>
#include <vector>
#include <exception>

#include <signal.h>
#include <cstdlib>

#include <qpid/sys/Time.h>
#include <qpid/agent/ManagementAgent.h>
#include <qpid/client/ConnectionSettings.h>

#include "qmf/org/matahariproject/Package.h"
#include "matahari/mh_agent.h"

extern "C" {
#include "matahari/logging.h"
}

using namespace qpid::management;
using namespace qpid::client;
using namespace std;
namespace _qmf = qmf::org::matahariproject;

// Global Variables
ManagementAgent::Singleton* singleton;

void
shutdown(int /*signal*/)
{
  exit(0);
}

#ifdef WIN32
#define BUFFER_SIZE 1024
static void
RegistryRead (HKEY hHive, wchar_t *szKeyPath, wchar_t *szValue, char **out)
{
    HKEY hKey;
    DWORD nSize = BUFFER_SIZE;
    wchar_t szData[BUFFER_SIZE];
    long lSuccess = RegOpenKey (hHive, szKeyPath, &hKey);
    
    if (lSuccess != ERROR_SUCCESS) {
	mh_debug("Could not open %ls key from the registry: %ld", szKeyPath, lSuccess);
	return;
    }
    
    lSuccess = RegQueryValueEx (hKey, szValue, NULL, NULL, (LPBYTE) szData, &nSize);
    if (lSuccess != ERROR_SUCCESS) {
	mh_debug("Could not read '%ls[%ls]' from the registry: %ld", szKeyPath, szValue, lSuccess);
	return;
    }
    mh_info("Obtained '%ls[%ls]' = '%ls' from the registry", szKeyPath, szValue, szData);
    if(out) {
	*out = (char *)malloc( BUFFER_SIZE );
	wcstombs(*out, szData, (size_t)BUFFER_SIZE);
    }
}
#else
struct option opt[] = {
    {"help", no_argument, NULL, 'h'},
    {"daemon", no_argument, NULL, 'd'},
    {"broker", required_argument, NULL, 'b'},
    {"gssapi", no_argument, NULL, 'g'},
    {"username", required_argument, NULL, 'u'},
    {"service", required_argument, NULL, 's'},
    {"port", required_argument, NULL, 'p'},
    {0, 0, 0, 0}
};

static void
print_usage()
{
    printf("Usage:\tmatahari-netd <options>\n");
    printf("\t-d | --daemon     run as a daemon.\n");
    printf("\t-h | --help       print this help message.\n");
    printf("\t-b | --broker     specify broker host name..\n");
    printf("\t-g | --gssapi     force GSSAPI authentication.\n");
    printf("\t-u | --username   username to use for authentication purproses.\n");
    printf("\t-s | --service    service name to use for authentication purproses.\n");
    printf("\t-p | --port       specify broker port.\n");
}
#endif

static gboolean 
mh_qpid_callback(int fd, gpointer user_data)
{
    ManagementAgent *agent = (ManagementAgent *)user_data;
    mh_trace("Qpid message recieved");
    agent->pollCallbacks();
    return TRUE;
}

static void
mh_qpid_disconnect(gpointer user_data)
{
    mh_err("Qpid connection closed");
}

int
MatahariAgent::init(int argc, char **argv, char* proc_name)
{
#ifdef WIN32
    char *value = NULL;
#else
    int arg;
    int idx = 0;
    bool daemonize = false;
#endif

    bool gssapi = false;
    char *servername = strdup(MATAHARI_BROKER);
    char *username = NULL;
    char *service = NULL;
    int serverport = MATAHARI_PORT;
    int debuglevel = 0;

    qpid::management::ConnectionSettings settings;
    ManagementAgent *agent;

    /* Set up basic logging */
    mh_log_init(proc_name, LOG_INFO, FALSE);    

#ifdef WIN32
    RegistryRead (
	HKEY_LOCAL_MACHINE,
	L"SYSTEM\\CurrentControlSet\\services\\Matahari",
	L"DebugLevel",
	&value);

    if(value) {
	debuglevel = atoi(value);
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

    if(value) {
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
    
#else
    
    // Get args
    while ((arg = getopt_long(argc, argv, "hdb:gu:s:p:v", opt, &idx)) != -1) {
	switch (arg) {
	    case 'h':
	    case '?':
		print_usage();
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
		    print_usage();
		    exit(1);
		}
		break;
	    case 'u':
		if (optarg) {
		    username = strdup(optarg);
		} else {
		    print_usage();
		    exit(1);
		}
		break;
	    case 'g':
		gssapi = true;
		break;
	    case 'p':
		if (optarg) {
		    serverport = atoi(optarg);
		} else {
		    print_usage();
		    exit(1);
		}
		break;
	    case 'b':
		if (optarg) {
		    servername = strdup(optarg);
		} else {
		    print_usage();
		    exit(1);
		}
		break;
	    default:
		fprintf(stderr, "unsupported option '-%c'.  See --help.\n", arg);
		print_usage();
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
#endif

    /* Re-initialize logging now that we've completed option processing */
    mh_log_init(proc_name, LOG_INFO+debuglevel, debuglevel > 0);

    // Get our management agent
    singleton = new ManagementAgent::Singleton();
    agent = singleton->getInstance();
    _qmf::Package packageInit(agent);

    // Set up the cleanup handler for sigint
    signal(SIGINT, shutdown);

    // Connect to the broker
    settings.host = servername;
    settings.port = serverport;

    if (username != NULL) {
	settings.username = username;
    }
    if (service != NULL) {
	settings.service = service;
    }
    if (gssapi == true) {
	settings.mechanism = "GSSAPI";
    }

    agent->setName("matahariproject.org", proc_name);
    std::string dataFile(".matahari-data-");
    agent->init(settings, 5, true, dataFile + proc_name);

    /* Do any setup required by our agent */
    if(this->setup(agent) < 0) {
	fprintf(stderr, "Failed to set up agent\n");
	return -1;
    } 
    

    this->mainloop = g_main_new(FALSE);
    this->qpid_source = mainloop_add_fd(
	G_PRIORITY_HIGH, agent->getSignalFd(), mh_qpid_callback, mh_qpid_disconnect, agent);

    return 0;
}

void
MatahariAgent::run()
{
    g_main_run(this->mainloop);
}
