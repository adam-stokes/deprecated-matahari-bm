#include <qpid/management/Manageable.h>
#include <qpid/management/ManagementObject.h>
#include <qpid/agent/ManagementAgent.h>

#include "qmf/com/redhat/nodereporter/CPU.h"

using namespace qpid::management;
using namespace std;

using qpid::management::Manageable;

class CPUWrapper : public Manageable
{
    friend ostream& operator<<(ostream &output, const CPUWrapper& cpu);
    int cpunum;
    int corenum;
    int numcores;

    int model;
    int family;
    int cpuid_lvl;
    double speed;
    int cache;

    string vendor;
    string flags;

    /* QMF related fields */
    ManagementAgent *agent;
    qmf::com::redhat::nodereporter::CPU *mgmt_object;

    void sync();
    
public:
    /* Constructors */
    CPUWrapper(ManagementAgent *agent__,
	       int cpunum__,
	       int coreid__,
	       int numcores__,
	       int model__,
	       int family__,
	       int cpuid_lvl__,
	       double speed__,
	       int cache__,
	       const string &vendor__,
	       const string &flags__);
    ~CPUWrapper();

    /* QMF Methods */
    ManagementObject* GetManagementObject(void) const { return mgmt_object; }

    status_t ManagementMethod(uint32_t methodId, Args& args, string& text) {
        return STATUS_NOT_IMPLEMENTED;
    }

    /* Field Accessors */
    int getCpunum() { return cpunum; }
    int getCorenum() { return corenum; }
    int getNumcores() { return numcores; }

    int getModel() { return model; }
    int getFamily() { return family; }
    int getCpuid_Lvl() { return cpuid_lvl; }
    double getSpeed() { return speed; }
    int getCache() { return cache; }

    const string &getVendor() { return vendor; }
    const string &getFlags() { return flags; }

#if 0
    void setCpunum(int cpunum__) { cpunum = cpunum__; }
    void setCorenum(int corenum__) { corenum = corenum__; }
    void setNumcores(int numcores__) { numcores = numcores__; }

    void setModel(int model__) { model = model__; }
    void setFamily(int family__) { family = family__; }
    void setCpuid_Lvl(int cpuid_lvl__) { cpuid_lvl = cpuid_lvl__; }
    void setSpeed(double speed__) { speed = speed__; }
    void setCache(int cache__) { cache = cache__; }

    void setVendor(const string &vendor__) { vendor = vendor__; }
    void setFlags(const string &flags__) { flags = flags__; } 
#endif
};

void fillCPUInfo(vector<CPUWrapper*> &cpus, ManagementAgent *agent);

