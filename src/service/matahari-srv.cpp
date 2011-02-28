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
#include "qmf/org/matahariproject/ArgsServicesList.h"
#include "qmf/org/matahariproject/ArgsServicesDescribe.h"
#include "qmf/org/matahariproject/ArgsServicesStop.h"
#include "qmf/org/matahariproject/ArgsServicesStart.h"
#include "qmf/org/matahariproject/ArgsServicesStatus.h"
#include "qmf/org/matahariproject/ArgsServicesEnable.h"
#include "qmf/org/matahariproject/ArgsServicesDisable.h"
#include "qmf/org/matahariproject/ArgsServicesCancel.h"
#include "qmf/org/matahariproject/ArgsServicesFail.h"
#include "qmf/org/matahariproject/EventService_op.h"

#include "qmf/org/matahariproject/Resources.h"
#include "qmf/org/matahariproject/ArgsResourcesList.h"
#include "qmf/org/matahariproject/ArgsResourcesList_classes.h"
#include "qmf/org/matahariproject/ArgsResourcesList_ocf_providers.h"
#include "qmf/org/matahariproject/ArgsResourcesDescribe.h"
#include "qmf/org/matahariproject/ArgsResourcesStop.h"
#include "qmf/org/matahariproject/ArgsResourcesStart.h"
#include "qmf/org/matahariproject/ArgsResourcesMonitor.h"
#include "qmf/org/matahariproject/ArgsResourcesInvoke.h"
#include "qmf/org/matahariproject/ArgsResourcesCancel.h"
#include "qmf/org/matahariproject/ArgsResourcesFail.h"
#include "qmf/org/matahariproject/EventResource_op.h"

extern "C" { 
#include "matahari/logging.h"
#include "matahari/services.h"
}

class SrvManageable : public Manageable
{
    private:
	_qmf::Services* _management_object;
	
    public:
	SrvManageable(ManagementAgent* agent) {
	    _management_object = new _qmf::Services(agent, this);
	    _management_object->set_hostname(matahari_hostname());
	    agent->addObject(this->_management_object);
	};
	ManagementObject* GetManagementObject() const { return _management_object; }
	status_t ManagementMethod(uint32_t method, Args& arguments, string& text);
};

class RscManageable : public Manageable
{
    private:
	_qmf::Resources* _management_object;
	
    public:
	RscManageable(ManagementAgent* agent) { 
	    _management_object = new _qmf::Resources(agent, this); 
	    _management_object->set_hostname(matahari_hostname());
	    _management_object->set_uuid(matahari_uuid());
	    agent->addObject(this->_management_object);
	};
	ManagementObject* GetManagementObject() const { return _management_object; }
	status_t ManagementMethod(uint32_t method, Args& arguments, string& text);
};

class SrvAgent : public MatahariAgent
{
    private:
	ManagementAgent* _agent;
	SrvManageable *services;
	RscManageable *resources;
	
    public:
	void raiseEvent(svc_action_t *op, int service);
	int setup(ManagementAgent* agent);
	ManagementObject* GetManagementObject() const { return services->GetManagementObject(); }
};


extern "C" { 
#include <stdlib.h>
#include <string.h>
};

SrvAgent agent;

static void mh_service_callback(svc_action_t *op)
{
    mh_trace("Completed: %s = %d\n", op->id, op->rc);
    agent.raiseEvent(op, 1);
}

static void mh_resource_callback(svc_action_t *op)
{
    mh_trace("Completed: %s = %d\n", op->id, op->rc);
    agent.raiseEvent(op, 0);
}

static GHashTable *qmf_map_to_hash(::qpid::types::Variant::Map parameters) 
{
    qpid::types::Variant::Map::const_iterator qIter;
    GHashTable *hash = g_hash_table_new_full(
	g_str_hash, g_str_equal, free, free);

    for (qIter=parameters.begin() ; qIter != parameters.end(); qIter++ ) {
	char *key = strdup(qIter->first.c_str());
	char *value = strdup(qIter->second.asString().c_str());	
	g_hash_table_insert(hash, key, value);
    }

    return hash;
}

int
main(int argc, char **argv)
{
    int rc = agent.init(argc, argv, "service");

    if(rc >= 0) {
	mainloop_track_children(G_PRIORITY_DEFAULT);
	agent.run();
    }
    
    return rc;
}

void SrvAgent::raiseEvent(svc_action_t *op, int service)
{
    uint64_t timestamp = 0L;

#ifndef MSVC
    timestamp = ::time(NULL);
#endif

    if(service) {
	this->_agent->raiseEvent(_qmf::EventService_op(
				     op->rsc, op->action, op->interval, op->rc, 0, timestamp));
    } else {
	this->_agent->raiseEvent(_qmf::EventResource_op(
				     op->rsc, op->action, op->interval, op->rc, 0, timestamp, 
				     op->agent, op->rclass, op->provider));
    }
}

int
SrvAgent::setup(ManagementAgent* agent)
{
    this->_agent = agent;
    this->services = new SrvManageable(agent);
    this->resources = new RscManageable(agent);

    return 0;
}

Manageable::status_t
SrvManageable::ManagementMethod(uint32_t method, Args& arguments, string& text)
{
    int default_timeout_ms = 60000;
    switch(method)
	{
	case _qmf::Services::METHOD_LIST:
	    {
		GList *gIter = NULL;
		GList *services = services_list();
		_qmf::ArgsServicesList& ioArgs = (_qmf::ArgsServicesList&) arguments;
		
		for(gIter = services; gIter != NULL; gIter = gIter->next) {
		    ioArgs.o_services.push_back((const char *)gIter->data);
		}
	    }
	    return Manageable::STATUS_OK;

	case _qmf::Services::METHOD_ENABLE:
	    {
		_qmf::ArgsServicesEnable& ioArgs = (_qmf::ArgsServicesEnable&) arguments;
		svc_action_t * op = services_action_create(ioArgs.i_name.c_str(), "enable", 0, default_timeout_ms);
		services_action_sync(op);
		services_action_free(op);
	    }
	    return Manageable::STATUS_OK;

	case _qmf::Services::METHOD_DISABLE:
	    {
		_qmf::ArgsServicesDisable& ioArgs = (_qmf::ArgsServicesDisable&) arguments;
		svc_action_t * op = services_action_create(ioArgs.i_name.c_str(), "disable", 0, default_timeout_ms);
		services_action_sync(op);
		services_action_free(op);
	    }
	    return Manageable::STATUS_OK;

	case _qmf::Services::METHOD_START:
	    {
		_qmf::ArgsServicesStart& ioArgs = (_qmf::ArgsServicesStart&) arguments;
		svc_action_t *op = services_action_create(ioArgs.io_name.c_str(), "start", 0, ioArgs.i_timeout);
		services_action_sync(op);
		ioArgs.o_rc = op->rc;
		services_action_free(op);
	    }
	    return Manageable::STATUS_OK;
	    
	case _qmf::Services::METHOD_STOP:
	    {
		_qmf::ArgsServicesStop& ioArgs = (_qmf::ArgsServicesStop&) arguments;
		svc_action_t * op = services_action_create(ioArgs.io_name.c_str(), "stop", 0, ioArgs.i_timeout);
		services_action_sync(op);
		ioArgs.o_rc = op->rc;
		services_action_free(op);
	    }
	    return Manageable::STATUS_OK;

	case _qmf::Services::METHOD_STATUS:
	    {
		_qmf::ArgsServicesStatus& ioArgs = (_qmf::ArgsServicesStatus&) arguments;
		svc_action_t *op = services_action_create(ioArgs.io_name.c_str(), "status", ioArgs.io_interval, ioArgs.i_timeout);

		if(ioArgs.io_interval) {
		    return Manageable::STATUS_NOT_IMPLEMENTED;

		    services_action_async(op, mh_service_callback);
		    ioArgs.o_rc = OCF_PENDING;

		} else {
		    services_action_sync(op);
		    ioArgs.o_rc = op->rc;
		    services_action_free(op);
		}
	    }	    
	    return Manageable::STATUS_OK;

	case _qmf::Services::METHOD_CANCEL:
	    {
		_qmf::ArgsServicesCancel& ioArgs = (_qmf::ArgsServicesCancel&) arguments;
		services_action_cancel(ioArgs.i_name.c_str(), ioArgs.i_action.c_str(), ioArgs.i_interval);
	    }
	    
	    return Manageable::STATUS_OK;

	// case _qmf::Services::METHOD_DESCRIBE:
	}
    return Manageable::STATUS_NOT_IMPLEMENTED;
}

Manageable::status_t
RscManageable::ManagementMethod(uint32_t method, Args& arguments, string& text)
{
    switch(method)
	{
	case _qmf::Resources::METHOD_LIST_CLASSES:
	    {
		_qmf::ArgsResourcesList_classes& ioArgs = (_qmf::ArgsResourcesList_classes&) arguments;
		ioArgs.o_classes.push_back("ocf");
		ioArgs.o_classes.push_back("lsb");
	    }
	    return Manageable::STATUS_OK;

	case _qmf::Resources::METHOD_LIST_OCF_PROVIDERS:
	    {
		GList *gIter = NULL;
		GList *providers = resources_list_ocf_providers();
		_qmf::ArgsResourcesList_ocf_providers& ioArgs = (_qmf::ArgsResourcesList_ocf_providers&) arguments;
		
		for(gIter = providers; gIter != NULL; gIter = gIter->next) {
		    ioArgs.o_providers.push_back((const char *)gIter->data);
		}
	    }
	    return Manageable::STATUS_OK;

	case _qmf::Resources::METHOD_LIST:
	    {
		GList *gIter = NULL;
		_qmf::ArgsResourcesList& ioArgs = (_qmf::ArgsResourcesList&) arguments;
		GList *agents = resources_list_ocf_agents(ioArgs.i_provider.c_str());
		
		for(gIter = agents; gIter != NULL; gIter = gIter->next) {
		    ioArgs.o_types.push_back((const char *)gIter->data);
		}
	    }
	    return Manageable::STATUS_OK;

	case _qmf::Resources::METHOD_START:
	    {
		_qmf::ArgsResourcesStart& ioArgs = (_qmf::ArgsResourcesStart&) arguments;
		GHashTable *params = qmf_map_to_hash(ioArgs.i_parameters);
		svc_action_t *op = resources_action_create(
		    ioArgs.io_name.c_str(), ioArgs.io_provider.c_str(), ioArgs.io_type.c_str(),
		    "start", 0, ioArgs.i_timeout, params);
		services_action_sync(op);
		ioArgs.o_rc = op->rc;
		services_action_free(op);
	    }
	    return Manageable::STATUS_OK;
	    
	case _qmf::Resources::METHOD_STOP:
	    {
		_qmf::ArgsResourcesStop& ioArgs = (_qmf::ArgsResourcesStop&) arguments;
		GHashTable *params = qmf_map_to_hash(ioArgs.i_parameters);
		svc_action_t *op = resources_action_create(
		    ioArgs.io_name.c_str(), ioArgs.io_provider.c_str(), ioArgs.io_type.c_str(),
		    "stop", 0, ioArgs.i_timeout, params);
		services_action_sync(op);
		ioArgs.o_rc = op->rc;
		services_action_free(op);
	    }
	    return Manageable::STATUS_OK;

	case _qmf::Resources::METHOD_MONITOR:
	    {
		_qmf::ArgsResourcesMonitor& ioArgs = (_qmf::ArgsResourcesMonitor&) arguments;
		GHashTable *params = qmf_map_to_hash(ioArgs.i_parameters);
		svc_action_t *op = resources_action_create(
		    ioArgs.io_name.c_str(), ioArgs.io_provider.c_str(), ioArgs.io_type.c_str(),
		    "monitor", ioArgs.io_interval, ioArgs.i_timeout, params);

		if(op->interval) {
		    return Manageable::STATUS_NOT_IMPLEMENTED;

		    services_action_async(op, mh_resource_callback);
		    ioArgs.o_rc = OCF_PENDING;

		} else {
		    services_action_sync(op);
		    ioArgs.o_rc = op->rc;
		    services_action_free(op);
		}
	    }	    
	    return Manageable::STATUS_OK;

	case _qmf::Resources::METHOD_INVOKE:
	    {
		_qmf::ArgsResourcesInvoke& ioArgs = (_qmf::ArgsResourcesInvoke&) arguments;
		GHashTable *params = qmf_map_to_hash(ioArgs.i_parameters);
		svc_action_t *op = resources_action_create(
		    ioArgs.io_name.c_str(), ioArgs.io_provider.c_str(), ioArgs.io_type.c_str(),
		    ioArgs.io_action.c_str(), ioArgs.io_interval, ioArgs.i_timeout, params);

		
		if(op->interval) {
		    return Manageable::STATUS_NOT_IMPLEMENTED;

		    services_action_async(op, mh_resource_callback);
		    ioArgs.o_rc = OCF_PENDING;

		} else {
		    services_action_sync(op);
		    ioArgs.o_rc = op->rc;
		    services_action_free(op);
		}
	    }	    
	    return Manageable::STATUS_OK;

	case _qmf::Resources::METHOD_CANCEL:
	    {
		_qmf::ArgsResourcesCancel& ioArgs = (_qmf::ArgsResourcesCancel&) arguments;
		services_action_cancel(ioArgs.io_name.c_str(), ioArgs.io_action.c_str(), ioArgs.io_interval);
	    }
	    
	    return Manageable::STATUS_OK;

	// case _qmf::Resources::METHOD_DESCRIBE:
	}
    return Manageable::STATUS_NOT_IMPLEMENTED;
}
