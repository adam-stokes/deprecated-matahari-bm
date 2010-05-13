/* processorsagent.cpp - Copyright (C) 2009 Red Hat, Inc.
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

#include "processorsagent.h"
#include <qpid/agent/ManagementAgent.h>

namespace _qmf = qmf::com::redhat::matahari;

ProcessorsAgent::ProcessorsAgent(Processors& processors)
  :_processors(processors)
{
  processors.addProcessorsListener(this);
}

ProcessorsAgent::~ProcessorsAgent()
{
}

void
ProcessorsAgent::setup(ManagementAgent* agent, HostAgent* parent)
{
  _management_object = new _qmf::Processors(agent, this, parent);
  agent->addObject(_management_object);

  _management_object->set_model(this->_processors.getModel());
  _management_object->set_cores(this->_processors.getNumberOfCores());
}

void
ProcessorsAgent::updated()
{
  _management_object->set_load_average(this->_processors.getLoadAverage());
}
