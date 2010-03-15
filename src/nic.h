/* nic.h - Copyright (C) 2009 Red Hat, Inc.
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

#include "qmf/com/redhat/matahari/NIC.h"
#include "hal.h"

using namespace qpid::management;
using namespace std;

using qpid::management::Manageable;

class NICWrapper : public Manageable
{
    friend ostream& operator <<(ostream &output, const NICWrapper& nic);
    friend class HostWrapper;

    // NIC Parameters
    string interfaceName;
    string macaddr;
    string ipaddr;
    string netmask;
    string broadcast;
    int bandwidth;

    // QMF related fields
    ManagementAgent *agent;
    qmf::com::redhat::matahari::NIC *mgmt_object;

    // Methods to put up / take down QMF Objects
    void setupQMFObject(ManagementAgent *agent, Manageable *parent);
    void cleanupQMFObject(void);
    void syncQMFObject(void);

    // Constructors and Destructor are private
    NICWrapper() {}
    NICWrapper(const NICWrapper&) {}
    ~NICWrapper() {}

    NICWrapper(const string &interfaceName__,
               const string &macaddr__,
               const string &ipaddr__,
               const string &netmask__,
               const string &broadcast__,
               int bandwidth__) {
        interfaceName = interfaceName__;
        macaddr = macaddr__;
        ipaddr = ipaddr__;
        netmask = netmask__;
        broadcast = broadcast__;
        bandwidth = bandwidth__;
    }

    static NICWrapper *getNIC(ManagementAgent *agent,
                       LibHalContext *hal_ctx,
                       char *nic_handle);

    int identifyNIC(int seconds);
public:

    // Factory like method
    static void fillNICInfo(vector<NICWrapper*> &nics,
                            ManagementAgent *agent,
                            LibHalContext *ctx);

    // QMF Methods
    ManagementObject *GetManagementObject(void) const { return mgmt_object; }
    status_t ManagementMethod(uint32_t methodId, Args& args, string& text);

    // Field Accessors
    const string &getInterfaceName(void) { return interfaceName; }
    const string &getMacaddr(void) { return macaddr; }
    const string &getIpaddr(void) { return ipaddr; }
    const string &getNetmask(void) { return netmask; }
    const string &getBroadcast(void) { return broadcast; }
    int getBandwidth(void) { return bandwidth; }
};
