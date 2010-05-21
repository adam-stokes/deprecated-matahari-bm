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

const string UNKNOWN("Unknow");

Host::Host()
  :_uuid(UNKNOWN)
  ,_hostname(UNKNOWN)
  ,_hypervisor(UNKNOWN)
  ,_architecture(UNKNOWN)
  ,_memory(0)
  ,_beeping(false)
  ,_heartbeat_sequence(0)
{
  struct utsname details;
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

void
Host::update()
{
  this->_heartbeat_sequence++;

  time_t __time;
  time(&__time);

  for(set<HostListener*>::iterator iter = _listeners.begin();
      iter != _listeners.end();
      iter++)
    {
      (*iter)->heartbeat((unsigned long)__time,
                         this->_heartbeat_sequence);
    }

  for(set<HostListener*>::iterator iter = _listeners.begin();
      iter != _listeners.end();
      iter++)
    {
      (*iter)->updated();
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

string
Host::getCPUModel() const
{
  return Platform::instance()->getCPUModel();
}

unsigned int
Host::getNumberOfCPUCores() const
{
  return Platform::instance()->getNumberOfCPUCores();
}

double
Host::getLoadAverage() const
{
  return Platform::instance()->getLoadAverage();
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
