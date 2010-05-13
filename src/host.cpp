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

#include "host.h"
#include "platform.h"

#include <fstream>
#include <libvirt/libvirt.h>
#include <stdexcept>
#include <string>
#include <sys/sysinfo.h>
#include <sys/utsname.h>

using namespace std;

Host::Host()
{
  struct utsname details;
  this->_uuid         = string("Unknown");
  this->_hostname     = string("Unknown");
  this->_hypervisor   = string("Unknown");
  this->_architecture = string("None");
  this->_memory       = 0;
  this->_beeping      = false;

  std::ifstream input("/var/lib/dbus/machine-id");

  if(input.is_open())
    {
      string uuid;

      getline(input, uuid);
      input.close();
      this->_uuid = uuid;
    }

  if(!uname(&details))
    {
      this->_hostname     = string(details.nodename);
      this->_architecture = string(details.machine);
    }
  else
    {
      throw runtime_error("Unable to retrieve system details");
    }

  virConnectPtr lvconn = virConnectOpenReadOnly(NULL);

  if(lvconn)
    {
      this->_hypervisor = string(virConnectGetType(lvconn));
      virConnectClose(lvconn);
    }

  struct sysinfo sysinf;
  if(!sysinfo(&sysinf))
    {
      this->_memory = sysinf.totalram / 1024L;
    }
  else
    {
      throw runtime_error("Unable to retrieve system memory details.");
    }
}

/*
void
Host::setup(ManagementAgent* agent, HostAgent* hostAgent)
{
  // discover the aspects of the host
  _processors.setup(agent, hostAgent);
  _networkdevices = Platform::instance()->get_network_devices();

  for(vector<NetworkDeviceAgent>::iterator iter = _networkdevices.begin();
      iter != _networkdevices.end();
      iter++)
    {
      iter->setup(agent, hostAgent);
    }
}
*/

void
Host::update()
{
  _processors.update();

  for(vector<NetworkDeviceAgent>::iterator iter = _networkdevices.begin();
      iter != _networkdevices.end();
      iter++)
    {
      iter->update();
    }
}

void
Host::addHostListener(HostListener* listener)
{
  _listeners.insert(listener);
}

void
Host::removeHostListener(HostListener* listener)
{
  _listeners.erase(listener);
}

Processors&
Host::getProcessors()
{
  return _processors;
}

string
Host:: getUUID() const
{
  return _uuid;
}

string
Host::getHostname() const
{
  return _hostname;
}

string
Host::getHypervisor() const
{
  return _hypervisor;
}

string
Host::getArchitecture() const
{
  return _architecture;
}

unsigned int
Host::getMemory() const
{
  return _memory;
}

bool
Host::isBeeping() const
{
  return _beeping;
}

void
Host::identify(const int iterations)
{
}

void
Host::shutdown()
{
}

void
Host::reboot()
{
}
