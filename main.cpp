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
#include "cpu.h"
#include "nic.h"
#include "qmf/com/redhat/nodereporter/Package.h"

using namespace qpid::management;
using namespace qpid::client;
using namespace std;
namespace _qmf = qmf::com::redhat::nodereporter;

// Global Variables
ManagementAgent::Singleton* singleton;
vector<CPUWrapper*> cpus;
vector<NICWrapper*> nics;

void clearCPUList()
{
    for (vector<CPUWrapper*>::iterator iter = cpus.begin(); iter != cpus.end();) {
        delete (*iter);
        iter = cpus.erase(iter);
    }
}

void clearNICList()
{
    for (vector<NICWrapper*>::iterator iter = nics.begin(); iter != nics.end();) {
        delete (*iter);
        iter = nics.erase(iter);
    }
}

void cleanup(void)
{
    delete singleton;
    clearCPUList();
    clearNICList();    
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
    LibHalContext *hal_ctx;

    // Get our HAL Context or die trying
    hal_ctx = get_hal_ctx();
    if (!hal_ctx)
        throw runtime_error("Unable to get HAL Context Structure.");

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
    	fillCPUInfo(cpus, agent);
	    fillNICInfo(nics, agent, hal_ctx);
    }
    catch (...) {
        put_hal_ctx(hal_ctx);
        cleanup();
	    throw;
    }

    // Close the Hal Context
    put_hal_ctx(hal_ctx);
    
    // Print gathered CPU data
    vector<CPUWrapper*>::iterator cpu_cursor = cpus.begin();
    while (cpu_cursor != cpus.end()) {
        cout << **cpu_cursor << endl;
        cpu_cursor++;
    }

    // Print gathered NIC data
    vector<NICWrapper*>::iterator nic_cursor = nics.begin();
    while (nic_cursor != nics.end()) {
        cout << **nic_cursor << endl;
        nic_cursor++;
    }

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
