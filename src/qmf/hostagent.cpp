/* hostagent.cpp - Copyright (C) 2009 Red Hat, Inc.
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

#include "hostagent.h"
#include <qpid/agent/ManagementAgent.h>

#include "qmf/com/redhat/matahari/EventHeartbeat.h"

HostAgent::HostAgent(Host& host)
  :_host(host)
{
  this->_host.addHostListener(this);
}

HostAgent::~HostAgent()
{
}

void
HostAgent::setup(ManagementAgent* agent)
{
  this->_agent = agent;

  _management_object = new _qmf::Host(agent, this);
  agent->addObject(_management_object);

  _management_object->set_uuid(_host.getUUID());
  _management_object->set_hostname(_host.getHostname());
  _management_object->set_hypervisor(_host.getHypervisor());
  _management_object->set_arch(_host.getArchitecture());
  _management_object->set_memory(_host.getMemory());
  _management_object->set_beeping(_host.isBeeping());
}

Manageable::status_t
HostAgent::ManagementMethod(uint32_t method, Args& arguments, string& text)
{
  switch(method)
    {
    case _qmf::Host::METHOD_SHUTDOWN:
      _host.shutdown();
      return Manageable::STATUS_OK;
    case _qmf::Host::METHOD_REBOOT:
      _host.reboot();
      return Manageable::STATUS_OK;
    }

  return Manageable::STATUS_NOT_IMPLEMENTED;
}

void
HostAgent::heartbeat(unsigned long timestamp, unsigned int sequence)
{
  this->_agent->raiseEvent(_qmf::EventHeartbeat(timestamp, sequence));
}
