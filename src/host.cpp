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

#include <qpid/management/Manageable.h>

#include <iostream>
#include <fstream>
#include <stdexcept>

#include <cstdlib>
#include <unistd.h>

#include <libvirt/libvirt.h>

#include "host.h"
#include "qmf/com/redhat/matahari/Host.h"

using namespace qpid::management;
using namespace std;

using qpid::management::Manageable;
namespace _qmf = qmf::com::redhat::matahari;

void
HostAgent::setup(ManagementAgent* agent)
{
  management_object = new _qmf::Host(agent, this);
  agent->addObject(management_object);

  // discover the aspects of the host
  processors.setup(agent, this);

  LibHalContext *hal_ctx;
  int ret;

  // Get our HAL Context or die trying
  hal_ctx = get_hal_ctx();
  if (!hal_ctx)
    throw runtime_error("Unable to get HAL Context Structure.");

  try {
    NICWrapper::fillNICInfo(this->nics, agent, hal_ctx);

    // Host UUID
    char *uuid_c = get_uuid(hal_ctx);
    string uuid(uuid_c);
    management_object->set_uuid(uuid);

    // Hostname
    char hostname_c[HOST_NAME_MAX];
    ret = gethostname(hostname_c, sizeof(hostname_c));
    if (ret != 0)
      throw runtime_error("Unable to get hostname");
    string hostname(hostname_c);
    management_object->set_hostname(hostname);

    // Hypervisor, arch, memory
    management_object->set_memory(0);
    management_object->set_hypervisor("unknown");
    management_object->set_arch("unknown");

    virConnectPtr connection;
    virNodeInfo info;
    connection = virConnectOpenReadOnly(NULL);
    if (connection) {
      const char *hv = virConnectGetType(connection);
      if (hv != NULL)
        management_object->set_hypervisor(hv);
      ret = virNodeGetInfo(connection, &info);
      if (ret == 0) {
        management_object->set_arch(info.model);
        management_object->set_memory(info.memory);
      }
    }
    virConnectClose(connection);

    management_object->set_beeping(false);

    // setup the nic objects
    for(vector<NICWrapper*>::iterator iter = nics.begin();
        iter != nics.end();
        iter++)
      {
        (*iter)->setupQMFObject(agent, this);
      }
  }
  catch (...) {
    put_hal_ctx(hal_ctx);
    throw;
  }

  // Close the Hal Context
  put_hal_ctx(hal_ctx);
}

void
HostAgent::update(void)
{
  processors.update();
}
