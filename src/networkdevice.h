#ifndef __NETWORKDEVICE_H
#define __NETWORKDEVICE_H

/* networkdevice.h - Copyright (C) 2010 Red Hat, Inc.
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

#include "qmf/com/redhat/matahari/NetworkDevice.h"

using namespace qpid::management;
using namespace std;

class NetworkDeviceAgent : public Manageable
{
 private:
  qmf::com::redhat::matahari::NetworkDevice* management_object;

  string ifname;
  string vendor;
  string model;
  string macaddr;

 public:
  NetworkDeviceAgent(string ifname, string vendor, string model, string macaddr);
  virtual ~NetworkDeviceAgent() {}

  string get_mac_address() const { return macaddr; }

  ManagementObject* GetManagementObject(void) const { return management_object; }

  void setup(ManagementAgent* agent, Manageable* parent);
  void update() const;
};

#endif
