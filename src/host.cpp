/* host.cpp - Copyright (C) 2009 Red Hat, Inc.
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

#include <fstream>

#include <libvirt/libvirt.h>
#include <qpid/management/Manageable.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>

#include "host.h"
#include "platform.h"
#include "qmf/com/redhat/matahari/Host.h"

using namespace qpid::management;
using namespace std;

using qpid::management::Manageable;
namespace _qmf = qmf::com::redhat::matahari;

void
HostAgent::setup(ManagementAgent* agent)
{
  management_object = new _qmf::Host(agent, this);
  agent->addObject(management_object);

  // discover the aspects of the host
  processors.setup(agent, this);
  networkdevices = Platform::instance()->get_network_devices();

  for(vector<NetworkDeviceAgent>::iterator iter = networkdevices.begin();
      iter != networkdevices.end();
      iter++)
    {
      iter->setup(agent, this);
    }

  struct utsname details;
  string uuid         = "Unknown";
  string hostname     = "Unknown";
  string hypervisor   = "Unknown";
  string architecture = "None";
  unsigned long memory = 0;
  bool beeping        = false;

  ifstream input("/var/lib/dbus/machine-id");

  if(input.is_open())
    {
      getline(input, uuid);
      input.close();
    }

  if(!uname(&details))
    {
      hostname = string(details.nodename);
      architecture = string(details.machine);
    }
  else
    {
      throw runtime_error("Unable to retrieve system details");
    }

  virConnectPtr lvconn = virConnectOpenReadOnly(NULL);

  if(lvconn)
    {
      hypervisor = string(virConnectGetType(lvconn));
      virConnectClose(lvconn);
    }

  struct sysinfo sysinf;
  if(!sysinfo(&sysinf))
    {
      memory = sysinf.totalram / 1024L;
    }
  else
    {
      throw runtime_error("Unable to retrieve system memory details.");
    }

  cout << "memory: " << memory << endl;

  management_object->set_uuid(uuid);
  management_object->set_hostname(hostname);
  management_object->set_hypervisor(hypervisor);
  management_object->set_arch(architecture);
  management_object->set_memory(memory);
  management_object->set_beeping(beeping);
}

void
HostAgent::update(void)
{
  processors.update();

  for(vector<NetworkDeviceAgent>::iterator iter = networkdevices.begin();
      iter != networkdevices.end();
      iter++)
    {
      iter->update();
    }
}
