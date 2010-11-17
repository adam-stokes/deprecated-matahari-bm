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

#ifndef WIN32
#include "config.h"
#endif


#include "mh_agent.h"
#include "qmf/com/redhat/matahari/Host.h"
#include "qmf/com/redhat/matahari/EventHeartbeat.h"

#include "host.h"
#include "processor.h"

class HostAgent : public MatahariAgent, public HostListener
{
    private:
	ManagementAgent* _agent;
	_qmf::Host* _management_object;

	void heartbeat(uint64_t timestamp, uint32_t sequence);
	void updated();	
	
    public:
	int setup(ManagementAgent* agent);
	ManagementObject* GetManagementObject() const { return _management_object; }
	status_t ManagementMethod(uint32_t method, Args& arguments, string& text);
};

int
main(int argc, char **argv)
{
    HostAgent agent; 
    int rc = agent.init(argc, argv);
    while (rc == 0) {
	qpid::sys::sleep(5);
	host_update_event();
    }
    
    return rc;
}

int
HostAgent::setup(ManagementAgent* agent)
{
  this->_agent = agent;
  host_register_listener(this);

  _management_object = new _qmf::Host(agent, this);
  agent->addObject(_management_object);

  _management_object->set_uuid(host_get_uuid());
  _management_object->set_hostname(host_get_hostname());
  _management_object->set_operating_system(host_get_operating_system());
  _management_object->set_hypervisor(host_get_hypervisor());
  _management_object->set_platform(host_get_platform());
  _management_object->set_arch(host_get_architecture());
  _management_object->set_memory(host_get_memory());
  _management_object->set_processors(cpu_get_count());
  _management_object->set_cores(cpu_get_number_of_cores());
  _management_object->set_model(cpu_get_model());
}

Manageable::status_t
HostAgent::ManagementMethod(uint32_t method, Args& arguments, string& text)
{
  switch(method)
    {
    case _qmf::Host::METHOD_SHUTDOWN:
      host_shutdown();
      return Manageable::STATUS_OK;
    case _qmf::Host::METHOD_REBOOT:
      host_reboot();
      return Manageable::STATUS_OK;
    }

  return Manageable::STATUS_NOT_IMPLEMENTED;
}

void
HostAgent::heartbeat(uint64_t timestamp, uint32_t sequence)
{
  uint64_t now = timestamp * 1000000000;
  this->_agent->raiseEvent(_qmf::EventHeartbeat(timestamp, sequence));
  _management_object->set_last_updated(now);
  _management_object->set_last_updated_seq(sequence);
}

void
HostAgent::updated()
{
  // update the load averages
  double one, five, fifteen;

  host_get_load_averages(one, five, fifteen);
  _management_object->set_load_average_1(one);
  _management_object->set_load_average_5(five);
  _management_object->set_load_average_15(fifteen);
}
