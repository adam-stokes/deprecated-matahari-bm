/* cpu.h - Copyright (C) 2009 Red Hat, Inc.
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

#include <qpid/management/Manageable.h>
#include <qpid/management/ManagementObject.h>
#include <qpid/agent/ManagementAgent.h>

#include "qmf/com/redhat/matahari/CPU.h"

using namespace qpid::management;
using namespace std;

using qpid::management::Manageable;

class CPUWrapper : public Manageable
{
    friend ostream& operator<<(ostream &output, const CPUWrapper& cpu);
    friend class HostWrapper;

    // CPU Parameters
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

    // QMF related fields
    ManagementAgent *agent;
    qmf::com::redhat::matahari::CPU *mgmt_object;

    // Methods to put up / take down QMF Objects
    void setupQMFObject(ManagementAgent *agent, Manageable *parent);
    void cleanupQMFObject(void);
    void syncQMFObject(void);

    // Constructors and Destructor are private
    CPUWrapper() {}
    CPUWrapper(const CPUWrapper&) {}
    ~CPUWrapper() {}

    CPUWrapper(int cpunum__,
               int corenum__,
               int numcores__,
               int model__,
               int family__,
               int cpuid_lvl__,
               double speed__,
               int cache__,
               const string &vendor__,
               const string &flags__) {
            cpunum = cpunum__;
        corenum = corenum__;
        numcores = numcores__;
        model = model__;
        family = family__;
        cpuid_lvl = cpuid_lvl__;
        speed = speed__;
        cache = cache__;
        vendor = vendor__;
        flags = flags__;
    }

public:

    // Factory like method
    static void fillCPUInfo(vector<CPUWrapper*> &cpus, ManagementAgent *agent);

    // QMF Methods
    ManagementObject* GetManagementObject(void) const { return mgmt_object; }

    status_t ManagementMethod(uint32_t methodId, Args& args, string& text) {
        return STATUS_NOT_IMPLEMENTED;
    }

    // Field Accessors
    int getCpunum(void) { return cpunum; }
    int getCorenum(void) { return corenum; }
    int getNumcores(void) { return numcores; }

    int getModel(void) { return model; }
    int getFamily(void) { return family; }
    int getCpuid_Lvl(void) { return cpuid_lvl; }
    double getSpeed(void) { return speed; }
    int getCache(void) { return cache; }

    const string &getVendor(void) { return vendor; }
    const string &getFlags(void) { return flags; }
};
