/* matahari-srv.cpp - Copyright (C) 2010 Red Hat, Inc.
 * Written by Andrew Beekhof <andrew@beekhof.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef WIN32
#include "config.h"
#endif

#include <string>
#include <qpid/management/Manageable.h>
#include <qpid/agent/ManagementAgent.h>
#include "matahari/mh_agent.h"

#include "qmf/org/matahariproject/Services.h"
#include "qmf/org/matahariproject/Resources.h"
#include "qmf/org/matahariproject/ArgsResourcesList.h"
#include "qmf/org/matahariproject/ArgsServicesList.h"
#include "qmf/org/matahariproject/ArgsServicesDescribe.h"
#include "qmf/org/matahariproject/ArgsServicesStop.h"
#include "qmf/org/matahariproject/ArgsServicesStart.h"
#include "qmf/org/matahariproject/ArgsServicesStatus.h"
#include "qmf/org/matahariproject/ArgsServicesEnable.h"
#include "qmf/org/matahariproject/ArgsServicesDisable.h"
#include "qmf/org/matahariproject/ArgsServicesCancel.h"
#include "qmf/org/matahariproject/ArgsServicesFail.h"

extern "C" { 
#include "matahari/logging.h"
}

class SrvAgent : public MatahariAgent
{
    private:
	ManagementAgent* _agent;
	_qmf::Services* _srv_management_object;
	_qmf::Resources* _rsc_management_object;
	
    public:
	int setup(ManagementAgent* agent);
	ManagementObject* GetManagementObject() const { return _srv_management_object; }
	status_t ManagementMethod(uint32_t method, Args& arguments, string& text);
};

extern "C" { 
#include <stdlib.h>
#include <string.h>
};

int
main(int argc, char **argv)
{
    SrvAgent agent;
    int rc = agent.init(argc, argv);
    while (rc == 0) {
	qpid::sys::sleep(1);
    }
    return rc;
}

int
SrvAgent::setup(ManagementAgent* agent)
{
    this->_agent = agent;
    this->_srv_management_object = new _qmf::Services(agent, this);
    this->_srv_management_object->set_hostname(get_hostname());

    agent->addObject(this->_srv_management_object);

    this->_rsc_management_object = new _qmf::Resources(agent, this);
    this->_rsc_management_object->set_hostname(get_hostname());

    agent->addObject(this->_rsc_management_object);
    return 0;
}

Manageable::status_t
SrvAgent::ManagementMethod(uint32_t method, Args& arguments, string& text)
{
    switch(method)
	{
	case _qmf::Resources::METHOD_LIST:
	    break;
	    
	case _qmf::Services::METHOD_LIST:
/*
	    {
		_qmf::ArgsServicesList& ioArgs = (_qmf::ArgsServicesList&) arguments;
		uint32_t lpc = 0;
		char **iface_list = NULL;
		ioArgs.o_max = ncf_num_of_interfaces(ncf, NETCF_IFACE_ACTIVE|NETCF_IFACE_INACTIVE);
		
		iface_list = (char**)calloc(ioArgs.o_max+1, sizeof(char*));
		if(ncf_list_interfaces(ncf, ioArgs.o_max, iface_list, NETCF_IFACE_ACTIVE|NETCF_IFACE_INACTIVE) < 0) {
		    ioArgs.o_max = 0;
		}
		
		for(lpc = 0; lpc < ioArgs.o_max; lpc++) {
		    nif = ncf_lookup_by_name(ncf, iface_list[lpc]);
		    ioArgs.o_iface_map.push_back(iface_list[lpc]);
		}
	    }
*/
	    return Manageable::STATUS_OK;

	case _qmf::Services::METHOD_START:
	    {
		// _qmf::ArgsServicesStart& ioArgs = (_qmf::ArgsServicesStart&) arguments;
	    }
	    return Manageable::STATUS_OK;
	    
	case _qmf::Services::METHOD_STOP:
	    {
		// _qmf::ArgsServicesStop& ioArgs = (_qmf::ArgsServicesStop&) arguments;
	    }
	    return Manageable::STATUS_OK;

	case _qmf::Services::METHOD_STATUS:
	    {
		// _qmf::ArgsServicesStatus& ioArgs = (_qmf::ArgsServicesStatus&) arguments;
	    }
	    return Manageable::STATUS_OK;

	case _qmf::Services::METHOD_DESCRIBE:
	    {
		// _qmf::ArgsServicesDescribe& ioArgs = (_qmf::ArgsServicesDescribe&) arguments;
	    }
	    return Manageable::STATUS_OK;

	}
    return Manageable::STATUS_NOT_IMPLEMENTED;
}
