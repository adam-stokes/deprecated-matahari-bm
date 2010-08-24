/* processoragent.cpp - Copyright (C) 2010 Red Hat, Inc.
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

#ifndef WIN32
#include "config.h"
#endif

#include "processoragent.h"
#include "processor.h"

ProcessorAgent::ProcessorAgent()
{ }

ProcessorAgent::~ProcessorAgent()
{ }

void
ProcessorAgent::setup(ManagementAgent* agent, HostAgent& parent)
{
  this->_agent          = agent;
  this->_management_object = new _qmf::Processor(agent, this);
  agent->addObject(this->_management_object);

  _management_object->set_host(parent.GetManagementObject()->getObjectId());
  _management_object->set_wordsize(cpu_get_wordsize());
  _management_object->set_model(cpu_get_model());
  _management_object->set_count(cpu_get_count());
  _management_object->set_cores(cpu_get_number_of_cores());
}

ManagementObject*
ProcessorAgent::GetManagementObject() const
{
  return this->_management_object;
}
