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

#ifndef WIN32
#include <config.h>
#include <getopt.h>
#endif

#ifdef WIN32
#include <windows.h>
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

#ifdef __linux__
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

int
MatahariAgent::init(int argc, char **argv)
{
#ifdef __linux__
    int arg;
    int idx = 0;
    bool verbose = false;
    bool daemonize = false;
#endif
    bool gssapi = false;
    char *servername = strdup(MATAHARI_BROKER);
    char *username = NULL;
    char *service = NULL;
    int serverport = MATAHARI_PORT;

    qpid::management::ConnectionSettings settings;
    ManagementAgent *agent;

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
    mh_log_init(basename(argv[0]), LOG_DEBUG);

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

    agent->init(settings, 5, false, ".magentdata");

    /* Do any setup required by our agent */
    if(this->setup(agent) < 0) {
	fprintf(stderr, "Failed to set up agent\n");
	return -1;
    } 
    
    return 0;
}
