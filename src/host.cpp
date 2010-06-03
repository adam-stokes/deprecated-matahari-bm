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
#include <set>

using namespace std;

set<HostListener*> _listeners;
unsigned int         _heartbeat_sequence;

void
host_register_listener(HostListener* listener)
{
  _listeners.insert(listener);
}

void
host_remove_listener(HostListener* listener)
{
  _listeners.erase(listener);
}

void
host_update_event()
{
  _heartbeat_sequence++;

  time_t __time;
  time(&__time);

  for(set<HostListener*>::iterator iter = _listeners.begin();
      iter != _listeners.end();
      iter++)
    {
      (*iter)->heartbeat((unsigned long)__time,
                         _heartbeat_sequence);
    }

  for(set<HostListener*>::iterator iter = _listeners.begin();
      iter != _listeners.end();
      iter++)
    {
      (*iter)->updated();
    }
}

string
host_get_uuid()
{
  return Platform::instance()->getUUID();
}

string
host_get_hostname()
{
  return Platform::instance()->getHostname();
}

string
host_get_hypervisor()
{
  return Platform::instance()->getHypervisor();
}

string
host_get_architecture()
{
  return Platform::instance()->getArchitecture();
}

unsigned int
host_get_memory()
{
  return Platform::instance()->getMemory();
}

string
host_get_cpu_model()
{
  return Platform::instance()->getCPUModel();
}

unsigned int
host_get_number_of_cpu_cores()
{
  return Platform::instance()->getNumberOfCPUCores();
}

double
host_get_load_average()
{
  return Platform::instance()->getLoadAverage();
}

void
host_reboot()
{
}

void
host_shutdown()
{
}
