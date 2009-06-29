#include <qpid/management/Manageable.h>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>

#include <cstdlib>
#include <unistd.h>

#include <libvirt/libvirt.h>

#include "host.h"
#include "qmf/com/redhat/nodereporter/Host.h"

using namespace qpid::management;
using namespace std;

using qpid::management::Manageable;
namespace _qmf = qmf::com::redhat::nodereporter;

ostream& operator<<(ostream &output, const HostWrapper& host)
{
    output << "Node" << endl << endl;
    output << "UUID: " << host.uuid << endl;
    output << "Hostname: " << host.hostname << endl;
    output << "Memory: " << host.memory << endl;
    output << "Hypervisor: " << host.hypervisor << endl;
    output << "Arch: " << host.arch << endl << endl;

    vector<CPUWrapper*> cpus = host.cpus;
    vector<NICWrapper*> nics = host.nics;

    for (vector<CPUWrapper*>::iterator iter = cpus.begin(); 
	 iter!= cpus.end();
	 iter++) {
        output << **iter << endl;
    }
    for (vector<NICWrapper*>::iterator iter = nics.begin(); 
	 iter!= nics.end();
	 iter++) {
        output << **iter << endl;
    }
    
    output << "End Node" << endl;
    return output;
}

void HostWrapper::setupQMFObjects(ManagementAgent *agent)
{
    // Set up Host object
    mgmt_object = new _qmf::Host(agent, this);
    agent->addObject(mgmt_object);
    syncQMFHostObject();

    // Iterate over list and set up CPU objects
    for (vector<CPUWrapper*>::iterator iter = cpus.begin(); 
	 iter!= cpus.end();
	 iter++) {
        (*iter)->setupQMFObject(agent, this);
    }
    // Iterate over list and set up NIC objects
    for (vector<NICWrapper*>::iterator iter = nics.begin(); 
	 iter!= nics.end();
	 iter++) {
        (*iter)->setupQMFObject(agent, this);
    }
}

void HostWrapper::syncQMFHostObject(void)
{
    mgmt_object->set_uuid(uuid);
    mgmt_object->set_hostname(hostname);
    mgmt_object->set_memory(memory);
    mgmt_object->set_beeping(beeping);
}

void HostWrapper::cleanupQMFObjects(void)
{
    // Clean up Host object
    mgmt_object->resourceDestroy();

    // Iterate over list and clean up CPU objects
    for (vector<CPUWrapper*>::iterator iter = cpus.begin(); 
	 iter!= cpus.end();
	 iter++) {
        (*iter)->cleanupQMFObject();
    }
    // Iterate over list and clean up NIC objects
    for (vector<NICWrapper*>::iterator iter = nics.begin(); 
	 iter!= nics.end();
	 iter++) {
        (*iter)->cleanupQMFObject();
    }
}

void HostWrapper::cleanupMemberObjects(void)
{
    // Get rid of the CPUWrapper objects for this host
    for (vector<CPUWrapper*>::iterator iter = cpus.begin(); iter != cpus.end();) {
        delete (*iter);
        iter = cpus.erase(iter);
    }
    // Get rid of the NICWrapper objects for this host
    for (vector<NICWrapper*>::iterator iter = nics.begin(); iter != nics.end();) {
        delete (*iter);
        iter = nics.erase(iter);
    }
}

void HostWrapper::disposeHostWrapper()
{
    if (hostSingleton == NULL)
        return;

    hostSingleton->cleanupQMFObjects();
    hostSingleton->cleanupMemberObjects();
    
    delete hostSingleton;
    hostSingleton = NULL;
}

HostWrapper* HostWrapper::setupHostWrapper(ManagementAgent *agent)
{
    if (hostSingleton != NULL)
        return hostSingleton;

    LibHalContext *hal_ctx;
    int ret;

    // Get our HAL Context or die trying
    hal_ctx = get_hal_ctx();
    if (!hal_ctx)
        throw runtime_error("Unable to get HAL Context Structure.");

    HostWrapper *host = new HostWrapper();

    try {
        CPUWrapper::fillCPUInfo(host->cpus, agent);
        NICWrapper::fillNICInfo(host->nics, agent, hal_ctx);

        // Host UUID
        char *uuid_c = get_uuid(hal_ctx);
        string uuid(uuid_c);
        host->uuid = uuid;

        // Hostname
        char hostname_c[HOST_NAME_MAX];
        ret = gethostname(hostname_c, sizeof(hostname_c));
        if (ret != 0)
            throw runtime_error("Unable to get hostname");
        string hostname(hostname_c);
        host->hostname = hostname;

        // Memory
        host->memory = 0;
        ifstream meminfo("/proc/meminfo", ios::in);
        if (!meminfo.is_open() || meminfo.fail())
            throw runtime_error("Unable to open /proc/cpuinfo");
        string line;
        getline(meminfo, line);
        meminfo.close();

        boost::smatch matches;
        boost::regex re("MemTotal:\\s*([\\S]+) kB");
        if (boost::regex_match(line, matches, re)) {
            host->memory = boost::lexical_cast<int>(matches[1]);
        }

	// Hypervisor, arch
	host->hypervisor = "unknown";
	host->arch = "unknown";

	virConnectPtr connection;
	virNodeInfo info;
	connection = virConnectOpenReadOnly(NULL);
	if (connection) {
	    const char *hv = virConnectGetType(connection);
	    if (hv != NULL)
		host->hypervisor = hv;
	    ret = virNodeGetInfo(connection, &info);
	    if (ret == 0)
		host->arch = info.model;
	}
	virConnectClose(connection);

        host->beeping = false;
    }
    catch (...) {
        host->cleanupMemberObjects();
        put_hal_ctx(hal_ctx);
        delete host;
        throw;
    }

    // Close the Hal Context
    put_hal_ctx(hal_ctx);

    host->setupQMFObjects(agent);

    // Setup singleton reference and return
    hostSingleton = host;
    return hostSingleton;
}

void HostWrapper::reboot()
{
    system("shutdown -r now");
}

void HostWrapper::shutdown()
{
    system("shutdown -h now");
}

Manageable::status_t 
HostWrapper::ManagementMethod(uint32_t methodId, Args& args, string& text) 
{
    switch(methodId) {
        case _qmf::Host::METHOD_SHUTDOWN:
	    shutdown();
	    return Manageable::STATUS_OK;
        case _qmf::Host::METHOD_REBOOT:
	    reboot();
	    return Manageable::STATUS_OK;
    }
    return Manageable::STATUS_NOT_IMPLEMENTED;
}
