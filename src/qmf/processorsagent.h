#ifndef __PROCESSORSAGENT_H
#define __PROCESSORSAGENT_H

/* processoragent.h - Copyright (C) 2010 Red Hat, Inc.
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

#include <qpid/management/Manageable.h>
#include <qpid/management/ManagementObject.h>

#include "processors.h"
#include "processorslistener.h"
#include "qmf/com/redhat/matahari/Processors.h"
#include "qmf/hostagent.h"

using namespace qpid::management;

class ProcessorsAgent : public Manageable, public ProcessorsListener
{
 private:
  qmf::com::redhat::matahari::Processors* _management_object;

  Processors& _processors;

 public:
  ProcessorsAgent(Processors& processors);
  virtual ~ProcessorsAgent();

  void setup(ManagementAgent* agent, HostAgent* parent);
  ManagementObject* GetManagementObject(void) const { return _management_object; }

  virtual void updated();
};

#endif
