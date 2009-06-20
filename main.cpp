#include <qpid/agent/ManagementAgent.h>
#include <qpid/client/ConnectionSettings.h>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <exception>

#include <signal.h>
#include <cstdlib>

#include "hal.h"
#include "host.h"
#include "qmf/com/redhat/nodereporter/Package.h"

using namespace qpid::management;
using namespace qpid::client;
using namespace std;
namespace _qmf = qmf::com::redhat::nodereporter;

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

int do_main(int argc, char **argv)
{
    const char* host = argc>1 ? argv[1] : "127.0.0.1";
    int port = argc>2 ? atoi(argv[2]) : 5672;
    ConnectionSettings settings;
    ManagementAgent *agent;
    HostWrapper *hostWrapper;

    // Get our management agent
    singleton = new ManagementAgent::Singleton();
    agent = singleton->getInstance();
    _qmf::Package packageInit(agent);

    // Set up the cleanup handler for sigint
    signal(SIGINT, shutdown);

    // Connect to the broker
    settings.host = host;
    settings.port = port;
    agent->init(settings, 5, false, ".magentdata");

    // Get the info and post it to the broker
    try {
        hostWrapper = HostWrapper::setupHostWrapper(agent);
    }
    catch (...) {
        cleanup();
	    throw;
    }

    cout << *hostWrapper << endl;

    // Keep alive while not EOF
    while(!cin.eof()) {
        cin.get();
    }

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
