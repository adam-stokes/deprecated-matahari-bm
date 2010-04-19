#ifndef __HOST_H
#define __HOST_H

/* host.h - Copyright (C) 2009 Red Hat, Inc.
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

#include "qmf/com/redhat/matahari/Host.h"

#include "nic.h"
#include "processors.h"

using namespace qpid::management;
using namespace std;

using qpid::management::Manageable;

class HostAgent : public Manageable
{
 private:
  qmf::com::redhat::matahari::Host* management_object;
  ProcessorsAgent processors;
  vector<NICWrapper*> nics;

 public:
  HostAgent() {}
  virtual ~HostAgent() {}

  ManagementObject* GetManagementObject(void) const { return management_object; }

  void setup(ManagementAgent* agent);
  void update(void);

  // agent methods
  void shutdown(void);
  void reboot(void);
};

#endif // __HOST_H
