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

#include "qmf/org/matahariproject/QmfPackage.h"

extern "C" {
#include "matahari/logging.h"
#include "matahari/services.h"
}

class SrvAgent : public MatahariAgent
{
    private:
        static void mh_service_callback(svc_action_t *op);
        static void mh_resource_callback(svc_action_t *op);
        void raiseEvent(svc_action_t *op, int service);

        qmf::Data _services;
        qmf::DataAddr _services_addr;

        qmf::Data _resources;
        qmf::DataAddr _resources_addr;

        gboolean invoke_services(qmf::AgentSession session, qmf::AgentEvent event, gpointer user_data);
        gboolean invoke_resources(qmf::AgentSession session, qmf::AgentEvent event, gpointer user_data);

        qmf::org::matahariproject::PackageDefinition _package;

    public:
        virtual int setup(qmf::AgentSession session);
        virtual gboolean invoke(qmf::AgentSession session, qmf::AgentEvent event,
                                gpointer user_data);
};


extern "C" {
#include <stdlib.h>
#include <string.h>
};

void
SrvAgent::mh_service_callback(svc_action_t *op)
{
    SrvAgent *agent = static_cast<SrvAgent *>(op->cb_data);
    mh_trace("Completed: %s = %d\n", op->id, op->rc);
    agent->raiseEvent(op, 1);
}

void
SrvAgent::mh_resource_callback(svc_action_t *op)
{
    SrvAgent *agent = static_cast<SrvAgent *>(op->cb_data);
    mh_trace("Completed: %s = %d\n", op->id, op->rc);
    agent->raiseEvent(op, 0);
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
    SrvAgent agent;
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
    qmf::Data event;

#ifndef MSVC
    timestamp = ::time(NULL);
#endif

    if(service) {
        event = qmf::Data(_package.event_service_op);
    } else {
        event = qmf::Data(_package.event_resource_op);
    }

    event.setProperty("name", op->rsc);
    event.setProperty("action", op->action);
    event.setProperty("interval", op->interval);
    event.setProperty("rc", op->rc);
    event.setProperty("timestamp", timestamp);
    event.setProperty("sequence", 0);


    if(service == 0) {
        event.setProperty("rsc_type", op->agent);
        event.setProperty("rsc_class", op->rclass);
        event.setProperty("rsc_provider", op->provider);
    }

    _agent_session.raiseEvent(event);
}

int
SrvAgent::setup(qmf::AgentSession session)
{
    _package.configure(session);
    _services = qmf::Data(_package.data_Services);

    _services.setProperty("uuid", mh_uuid());
    _services.setProperty("hostname", mh_hostname());

    _services_addr = session.addData(_services);

    _resources = qmf::Data(_package.data_Resources);

    _resources.setProperty("uuid", mh_uuid());
    _resources.setProperty("hostname", mh_hostname());

    _resources_addr = session.addData(_resources);

    return 0;
}

gboolean
SrvAgent::invoke(qmf::AgentSession session, qmf::AgentEvent event, gpointer user_data)
{
    if(event.getType() == qmf::AGENT_METHOD && event.hasDataAddr()) {
        if(_services_addr == event.getDataAddr()) {
            mh_info("Calling services API");
            return invoke_services(session, event, user_data);
        }

        mh_info("Calling resources API");
        return invoke_resources(session, event, user_data);
    }

    mh_err("Unhandled message");
    return TRUE;
}


gboolean
SrvAgent::invoke_services(qmf::AgentSession session, qmf::AgentEvent event, gpointer user_data)
{
    static const int default_timeout_ms = 60000;
    const std::string& methodName(event.getMethodName());
    if(event.getType() != qmf::AGENT_METHOD) {
        return TRUE;
    }

    if (methodName == "list") {
        _qtype::Variant::List s_list;
        GList *gIter = NULL;
        GList *services = services_list();

        for(gIter = services; gIter != NULL; gIter = gIter->next) {
            s_list.push_back((const char *)gIter->data);
        }

        event.addReturnArgument("services", s_list);

    } else if (methodName == "enable") {
        svc_action_t * op = services_action_create(
            event.getArguments()["name"].asString().c_str(), "enable", 0, default_timeout_ms);
        services_action_sync(op);
        services_action_free(op);

    } else if (methodName == "disable") {
        svc_action_t * op = services_action_create(
            event.getArguments()["name"].asString().c_str(), "disable", 0, default_timeout_ms);
        services_action_sync(op);
        services_action_free(op);

    } else if (methodName == "start") {
        svc_action_t *op = services_action_create(
            event.getArguments()["name"].asString().c_str(), "start", 0, event.getArguments()["timeout"]);
        services_action_sync(op);
        event.addReturnArgument("rc", op->rc);
        services_action_free(op);

    } else if (methodName == "stop") {
        svc_action_t * op = services_action_create(
            event.getArguments()["name"].asString().c_str(), "stop", 0, event.getArguments()["timeout"]);
        services_action_sync(op);
        event.addReturnArgument("rc", op->rc);
        services_action_free(op);

    } else if (methodName == "status") {
        svc_action_t *op = services_action_create(
            event.getArguments()["name"].asString().c_str(), "status",
            event.getArguments()["interval"], event.getArguments()["timeout"]);

        if(event.getArguments()["interval"]) {
            session.raiseException(event, MH_NOT_IMPLEMENTED);
            return TRUE;

            op->cb_data = this;
            services_action_async(op, mh_service_callback);
            event.addReturnArgument("rc", OCF_PENDING);

        } else {
            services_action_sync(op);
            event.addReturnArgument("rc", op->rc);
            services_action_free(op);
        }

    } else if (methodName == "cancel") {
        services_action_cancel(
            event.getArguments()["name"].asString().c_str(), event.getArguments()["action"].asString().c_str(),
            event.getArguments()["interval"]);

    } else {
        session.raiseException(event, MH_NOT_IMPLEMENTED);
        return TRUE;
    }

    session.methodSuccess(event);
    return TRUE;
}

gboolean
SrvAgent::invoke_resources(qmf::AgentSession session, qmf::AgentEvent event, gpointer user_data)
{
    const std::string& methodName(event.getMethodName());
    if(event.getType() != qmf::AGENT_METHOD) {
        return TRUE;
    }

    if (methodName == "list_classes") {
        _qtype::Variant::List c_list;
        c_list.push_back("ocf");
        c_list.push_back("lsb");
        event.addReturnArgument("classes", c_list);

    } else if (methodName == "list_ocf_providers") {
        GList *gIter = NULL;
        GList *providers = resources_list_ocf_providers();
        _qtype::Variant::List p_list;

        for(gIter = providers; gIter != NULL; gIter = gIter->next) {
            p_list.push_back((const char *)gIter->data);
        }
        event.addReturnArgument("providers", p_list);

    } else if (methodName == "list") {
        GList *gIter = NULL;
        GList *agents = resources_list_ocf_agents(event.getArguments()["provider"].asString().c_str());
        _qtype::Variant::List t_list;

        for(gIter = agents; gIter != NULL; gIter = gIter->next) {
            t_list.push_back((const char *)gIter->data);
        }
        event.addReturnArgument("types", t_list);

    } else if (methodName == "start") {
        GHashTable *params = qmf_map_to_hash(event.getArguments()["parameters"].asMap());
        svc_action_t *op = resources_action_create(
            event.getArguments()["name"].asString().c_str(), event.getArguments()["provider"].asString().c_str(),
            event.getArguments()["type"].asString().c_str(), "start", 0, event.getArguments()["timeout"], params);
        services_action_sync(op);
        event.addReturnArgument("rc", op->rc);
        services_action_free(op);

    } else if (methodName == "stop") {
        GHashTable *params = qmf_map_to_hash(event.getArguments()["parameters"].asMap());
        svc_action_t *op = resources_action_create(
            event.getArguments()["name"].asString().c_str(), event.getArguments()["provider"].asString().c_str(),
            event.getArguments()["type"].asString().c_str(), "stop", 0, event.getArguments()["timeout"], params);
        services_action_sync(op);
        event.addReturnArgument("rc", op->rc);
        services_action_free(op);

    } else if (methodName == "monitor") {
        GHashTable *params = qmf_map_to_hash(event.getArguments()["parameters"].asMap());
        svc_action_t *op = resources_action_create(
            event.getArguments()["name"].asString().c_str(), event.getArguments()["provider"].asString().c_str(),
            event.getArguments()["type"].asString().c_str(), "monitor", event.getArguments()["interval"],
            event.getArguments()["timeout"], params);

        if(op->interval) {
            session.raiseException(event, MH_NOT_IMPLEMENTED);
            return TRUE;

            op->cb_data = this;
            services_action_async(op, mh_resource_callback);
            event.addReturnArgument("rc", OCF_PENDING);

        } else {
            services_action_sync(op);
            event.addReturnArgument("rc", op->rc);
            services_action_free(op);
        }
    } else if (methodName == "invoke") {
        GHashTable *params = qmf_map_to_hash(event.getArguments()["parameters"].asMap());
        svc_action_t *op = resources_action_create(
            event.getArguments()["name"].asString().c_str(), event.getArguments()["provider"].asString().c_str(),
            event.getArguments()["type"].asString().c_str(), event.getArguments()["action"].asString().c_str(),
            event.getArguments()["interval"], event.getArguments()["timeout"], params);

        if(op->interval) {
            session.raiseException(event, MH_NOT_IMPLEMENTED);
            return TRUE;

            op->cb_data = this;
            services_action_async(op, mh_resource_callback);
            event.addReturnArgument("rc", OCF_PENDING);

        } else {
            services_action_sync(op);
            event.addReturnArgument("rc", op->rc);
            services_action_free(op);
        }
    } else if (methodName == "cancel") {
        services_action_cancel(
            event.getArguments()["name"].asString().c_str(), event.getArguments()["action"].asString().c_str(),
            event.getArguments()["interval"]);

    } else {
        session.raiseException(event, MH_NOT_IMPLEMENTED);
        return TRUE;
    }

    session.methodSuccess(event);
    return TRUE;
}
