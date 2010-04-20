/* networkingdevice.cpp - Copyright (C) 2010 Red Hat, Inc.
 * Written by Darryl L. Pierce <dpierce@redhat.com>
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

#include <qpid/agent/ManagementAgent.h>
#include <qpid/management/Manageable.h>

#include "networkdevice.h"

namespace _qmf = qmf::com::redhat::matahari;

NetworkDeviceAgent::NetworkDeviceAgent(string ifname, string vendor, string model, string macaddr)
  :ifname(ifname)
  ,vendor(vendor)
  ,model(model)
  ,macaddr(macaddr)
{}

void
NetworkDeviceAgent::setup(ManagementAgent* agent, Manageable* parent)
{
  management_object = new _qmf::NetworkDevice(agent, this, parent);
  agent->addObject(management_object);

  management_object->set_interface(ifname);
  management_object->set_vendor(vendor);
  management_object->set_model(model);
  management_object->set_mac_address(macaddr);
}

void
NetworkDeviceAgent::update() const
{
}
