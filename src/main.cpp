/* Copyright (C) 2009 Red Hat, Inc.
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

#include <qpid/agent/ManagementAgent.h>
#include <qpid/client/ConnectionSettings.h>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <exception>

#include <signal.h>
#include <cstdlib>

#include <getopt.h>

#include "hal.h"
#include "host.h"
#include "qmf/com/redhat/matahari/Package.h"

using namespace qpid::management;
using namespace qpid::client;
using namespace std;
namespace _qmf = qmf::com::redhat::matahari;

// Global Variables
ManagementAgent::Singleton* singleton;
HostWrapper* HostWrapper::hostSingleton = NULL;

void cleanup(void)
{
    HostWrapper::disposeHostWrapper();
    delete singleton;
}

void shutdown(int)
{
    cleanup();
    exit(0);
}

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

int do_main(int argc, char **argv)
{
    int arg;
    int idx = 0;
    bool daemonize = false;
    bool gssapi = false;
    bool verbose = false;
    char *host = NULL;
    char *username = NULL;
    char *service = NULL;
    int port = 5672;

    ConnectionSettings settings;
    ManagementAgent *agent;
    HostWrapper *hostWrapper;

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
                    port = atoi(optarg);
                } else {
                    print_usage();
                    exit(1);
                }
                break;
            case 'b':
                if (optarg) {
                    host = strdup(optarg);
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

    // Get our management agent
    singleton = new ManagementAgent::Singleton();
    agent = singleton->getInstance();
    _qmf::Package packageInit(agent);

    // Set up the cleanup handler for sigint
    signal(SIGINT, shutdown);

    // Connect to the broker
    settings.host = host ? host : "127.0.0.1";
    settings.port = port;

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
    try {
        hostWrapper = HostWrapper::setupHostWrapper(agent);
    }
    catch (...) {
        cleanup();
	    throw;
    }

    // Main loop
    hostWrapper->doLoop();

    // And we are done
    cleanup();
    return 0;
}

int main(int argc, char** argv)
{
    try {
        return do_main(argc, argv);
    } 
    catch(std::exception& e) {
        cout << "Top Level Exception: " << e.what() << endl;
    }
}