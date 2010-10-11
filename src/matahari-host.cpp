/* main.cpp - Copyright (C) 2009 Red Hat, Inc.
 * Written by Arjun Roy <arroy@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.  A copy of the GNU General Public License is
 * also available at http://www.gnu.org/copyleft/gpl.html.
 */

#ifndef WIN32
#include <config.h>
#include <getopt.h>
#endif

#ifdef WIN32
#include <windows.h>
#endif

#include <errno.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <exception>

#include <signal.h>
#include <cstdlib>

#include "host.h"

#include <qpid/sys/Time.h>
#include <qpid/agent/ManagementAgent.h>
#include <qpid/client/ConnectionSettings.h>

#include "qmf/hostagent.h"
#include "qmf/processoragent.h"

#include "qmf/com/redhat/matahari/host/Package.h"

using namespace qpid::management;
using namespace qpid::client;
using namespace std;
namespace _qmf = qmf::com::redhat::matahari::host;

// Global Variables
ManagementAgent::Singleton* singleton;

void
shutdown(int /*signal*/)
{
  exit(0);
}

#ifdef __linux__
static void
print_usage()
{
    printf("Usage:\tmatahari <options>\n");
    printf("\t-d | --daemon     run as a daemon.\n");
    printf("\t-h | --help       print this help message.\n");
    printf("\t-b | --broker     specify broker host name..\n");
    printf("\t-g | --gssapi     force GSSAPI authentication.\n");
    printf("\t-u | --username   username to use for authentication purproses.\n");
    printf("\t-s | --service    service name to use for authentication purproses.\n");
    printf("\t-p | --port       specify broker port.\n");
}
#endif

int
main(int argc, char **argv)
{
#ifdef __linux__
    int arg;
    int idx = 0;
    bool verbose = false;
    bool daemonize = false;
#endif
    bool gssapi = false;
    char *servername = NULL;
    char *username = NULL;
    char *service = NULL;
    int serverport = 5672;

#if QPID_VERSION < 07
    ConnectionSettings settings;
#else
    qpid::management::ConnectionSettings settings;
#endif
    ManagementAgent *agent;
    HostAgent hostAgent;
    ProcessorAgent processorAgent;

#ifdef __linux__
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

    // Get args
    while ((arg = getopt_long(argc, argv, "hdb:gu:s:p:", opt, &idx)) != -1) {
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
		verbose = true;
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

    // Get our management agent
    singleton = new ManagementAgent::Singleton();
    agent = singleton->getInstance();
    _qmf::Package packageInit(agent);

    // Set up the cleanup handler for sigint
    signal(SIGINT, shutdown);

    // Connect to the broker
    settings.host = servername ? servername : "127.0.0.1";
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

    agent->init(settings, 5, false, ".magentdata");

    // Get the info and post it to the broker
    hostAgent.setup(agent);
    processorAgent.setup(agent, hostAgent);

    while(1)
      {
	host_update_event();
	qpid::sys::sleep(5);
      }

    return 0;
}
