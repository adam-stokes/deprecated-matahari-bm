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
#include "matahari/services.h"
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
/*
  Seems we can only manage one object/class per agent...
    this->_rsc_management_object = new _qmf::Resources(agent, this);
    this->_rsc_management_object->set_hostname(get_hostname());

    agent->addObject(this->_rsc_management_object);
*/
    return 0;
}

Manageable::status_t
SrvAgent::ManagementMethod(uint32_t method, Args& arguments, string& text)
{
    int default_timeout_ms = 60000;
    switch(method)
	{
	case _qmf::Services::METHOD_LIST:
	    {
		GList *gIter = NULL;
		GList *services = list_services();
		_qmf::ArgsServicesList& ioArgs = (_qmf::ArgsServicesList&) arguments;
		
		for(gIter = services; gIter != NULL; gIter = gIter->next) {
		    ioArgs.o_services.push_back(gIter->data);
		}
	    }
	    return Manageable::STATUS_OK;

	case _qmf::Services::METHOD_ENABLE:
	    {
		_qmf::ArgsServicesEnable& ioArgs = (_qmf::ArgsServicesEnable&) arguments;
		rsc_op_t * op = create_service_op(ioArgs.i_name.c_str(), "enable", 0, default_timeout_ms);
		perform_sync_action(op);
		free_operation(op);
	    }
	    return Manageable::STATUS_OK;

	case _qmf::Services::METHOD_DISABLE:
	    {
		_qmf::ArgsServicesDisable& ioArgs = (_qmf::ArgsServicesDisable&) arguments;
		rsc_op_t * op = create_service_op(ioArgs.i_name.c_str(), "disable", 0, default_timeout_ms);
		perform_sync_action(op);
		free_operation(op);
	    }
	    return Manageable::STATUS_OK;

	case _qmf::Services::METHOD_START:
	    {
		_qmf::ArgsServicesStart& ioArgs = (_qmf::ArgsServicesStart&) arguments;
		rsc_op_t * op = create_service_op(ioArgs.io_name.c_str(), "start", 0, ioArgs.i_timeout);
		perform_sync_action(op);
		free_operation(op);
	    }
	    return Manageable::STATUS_OK;
	    
	case _qmf::Services::METHOD_STOP:
	    {
		_qmf::ArgsServicesStop& ioArgs = (_qmf::ArgsServicesStop&) arguments;
		rsc_op_t * op = create_service_op(ioArgs.io_name.c_str(), "stop", 0, ioArgs.i_timeout);
		perform_sync_action(op);
		free_operation(op);
	    }
	    return Manageable::STATUS_OK;

	case _qmf::Services::METHOD_STATUS:
	    {
		_qmf::ArgsServicesStatus& ioArgs = (_qmf::ArgsServicesStatus&) arguments;
		rsc_op_t * op = create_service_op(ioArgs.io_name.c_str(), "status", ioArgs.io_interval, ioArgs.i_timeout);
		perform_sync_action(op);
		free_operation(op);
	    }
	    return Manageable::STATUS_OK;

	case _qmf::Services::METHOD_CANCEL:
	    /* We don't support recurring or asynchronous actions yet */
	    return Manageable::STATUS_NOT_IMPLEMENTED;

	case _qmf::Services::METHOD_DESCRIBE:
	    /* We don't support describe yet */
	    return Manageable::STATUS_NOT_IMPLEMENTED;

	}
    return Manageable::STATUS_NOT_IMPLEMENTED;
}
