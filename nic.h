#include <qpid/management/Manageable.h>
#include <qpid/management/ManagementObject.h>
#include <qpid/agent/ManagementAgent.h>

#include "qmf/com/redhat/nodereporter/NIC.h"

using namespace qpid::management;
using namespace std;

using qpid::management::Manageable;

class NICWrapper : public Manageable
{
    friend ostream& operator <<(ostream &output, const NICWrapper& nic);
    string interfaceName;
    string macaddr;
    string ipaddr;
    string netmask;
    string broadcast;
    int bandwidth;

    /* QMF related fields */
    ManagementAgent *agent;
    qmf::com::redhat::nodereporter::NIC *mgmt_object;

    void sync();

public:
    /* Constructors */
    NICWrapper(ManagementAgent *agent__,
	       const string &interfaceName__,
	       const string &macaddr__,
	       const string &ipaddr__,
	       const string &netmask__,
	       const string &broadcast__,
	       int bandwidth__);
    ~NICWrapper();

    /* QMF Methods */
    ManagementObject *GetManagementObject(void) const { return mgmt_object; }

    status_t ManagementMethod(uint32_t methodId, Args& args, string& text) {
        return STATUS_NOT_IMPLEMENTED;
    }

    /* Field Accessors */
    const string &getInterfaceName() { return interfaceName; }
    const string &getMacaddr() { return macaddr; }
    const string &getIpaddr() { return ipaddr; }
    const string &getNetmask() { return netmask; }
    const string &getBroadcast() { return broadcast; }
    int getBandwidth() { return bandwidth; }

#if 0
    void setInterfaceName(const string &interfaceName__) {
      interfaceName = interfaceName__;
    }
    void setMacaddr(const string &macaddr__) { macaddr = macaddr__; }
    void setIpaddr(const string &ipaddr__) { ipaddr = ipaddr__; }
    void setNetmask(const string &netmast__) { netmask = netmask__; }
    void setBroadcast(const string &broadcast__) { broadcast = broadcast__; }
    void setBandwidth(int bandwidth__) { bandwidth = bandwidth__; }
#endif
};

void fillNICInfo(vector<NICWrapper*> &nics, 
		 ManagementAgent *agent, 
		 LibHalContext *ctx);

