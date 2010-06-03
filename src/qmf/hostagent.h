#ifndef __HOSTAGENT_H
#define __HOSTAGENT_H

/* hostagent.h - Copyright (C) 2009 Red Hat, Inc.
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
#include <string>

#include "host.h"
#include "hostlistener.h"

#include "qmf/com/redhat/matahari/Host.h"

using namespace qpid::management;
using namespace std;

namespace _qmf = qmf::com::redhat::matahari;

class HostAgent : public Manageable, public HostListener
{
 private:
  _qmf::Host*      _management_object;
  ManagementAgent* _agent;

 public:
  HostAgent();
  virtual ~HostAgent();

  void setup(ManagementAgent* agent);
  ManagementObject* GetManagementObject() const { return _management_object; }
  status_t ManagementMethod(uint32_t method, Args& arguments, string& text);

  virtual void heartbeat(unsigned long timestamp, unsigned int sequence);

  virtual void updated();
};

#endif
