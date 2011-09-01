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

extern "C" {
#include <stdlib.h>
#include <string.h>
};

#include <string>
#include <qpid/management/Manageable.h>
#include <qpid/agent/ManagementAgent.h>
#include "matahari/agent.h"

#include "qmf/org/matahariproject/QmfPackage.h"

extern "C" {
#include "matahari/logging.h"
#include "matahari/services.h"
}

#include <iostream>

enum service_id {
    SRV_RESOURCES,
    SRV_SERVICES
};

class SrvAgent : public MatahariAgent
{
private:
    void action_async(enum service_id service, qmf::AgentSession& session,
                      qmf::AgentEvent& event, svc_action_t *op, bool has_rc);

    qmf::Data _services;
    static const char SERVICES_NAME[];

    qmf::Data _resources;
    static const char RESOURCES_NAME[];

    _qtype::Variant::List standards;

    gboolean invoke_services(qmf::AgentSession session,
                             qmf::AgentEvent event, gpointer user_data);
    gboolean invoke_resources(qmf::AgentSession session,
                              qmf::AgentEvent event, gpointer user_data);

    qmf::org::matahariproject::PackageDefinition _package;

public:
    virtual int setup(qmf::AgentSession session);
    virtual gboolean invoke(qmf::AgentSession session,
                            qmf::AgentEvent event, gpointer user_data);
    void raiseEvent(svc_action_t *op, enum service_id service, const std::string &userdata);
};

const char SrvAgent::SERVICES_NAME[] = "Services";

const char SrvAgent::RESOURCES_NAME[] = "Resources";

/**
 * Async process callback
 *
 * This class is used to store off some data that will be needed in the
 * callback for a result from an asynchronous services API call being executed.
 */
class AsyncCB {
public:
    AsyncCB(SrvAgent *_agent, enum service_id _service,
            qmf::AgentSession& _session, qmf::AgentEvent& _event,
            bool _has_rc) :
            agent(_agent), service(_service), session(_session), event(_event),
            has_rc(_has_rc), last_rc(0), first_result(true) {};
    ~AsyncCB() {};

    static void mh_async_callback(svc_action_t *op);

    /** Cached SrvAgent instance */
    SrvAgent *agent;
    /** Which service this callback is associated with */
    enum service_id service;
    /** The QMF session that initiated this async action */
    qmf::AgentSession session;
    /** The method call that initiated this async action */
    qmf::AgentEvent event;
    /** Whether or not this method has an rc output param. */
    bool has_rc;

    /** The last result code for recurring actions */
    int last_rc;
    /** true if this is the first callback. */
    bool first_result;
};

void
AsyncCB::mh_async_callback(svc_action_t *op)
{
    std::string userdata;
    AsyncCB *cb_data = static_cast<AsyncCB *>(op->cb_data);

    mh_trace("Completed: %s = %d", op->id, op->rc);

    qpid::types::Variant::Map& args = cb_data->event.getArguments();
    if (args.count("userdata") > 0) {
        userdata = args["userdata"].asString();
    }

    if (cb_data->first_result) {
        if (cb_data->has_rc) {
            cb_data->event.addReturnArgument("rc", op->rc);
        }
        if (userdata.length()) {
            cb_data->event.addReturnArgument("userdata", userdata);
        }
        cb_data->session.methodSuccess(cb_data->event);
        cb_data->first_result = false;

    } else if (cb_data->last_rc != op->rc) {
        mh_trace("Result changed on recurring action: was '%d', now '%d'",
                 cb_data->last_rc, op->rc);
        cb_data->agent->raiseEvent(op, cb_data->service, userdata);
    }

    if (op->interval) { /* recurring action */
        cb_data->last_rc = op->rc;
    } else {
        delete cb_data;
        op->cb_data = NULL;
    }
}

static GHashTable *
qmf_map_to_hash(::qpid::types::Variant::Map parameters)
{
    qpid::types::Variant::Map::const_iterator qIter;
    GHashTable *hash = g_hash_table_new_full(
        g_str_hash, g_str_equal, free, free);

    for (qIter = parameters.begin(); qIter != parameters.end(); qIter++) {
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

    if (rc >= 0) {
        mainloop_track_children(G_PRIORITY_DEFAULT);
        agent.run();
    }

    return rc;
}

void
SrvAgent::raiseEvent(svc_action_t *op, enum service_id service, const std::string &userdata)
{
    uint64_t timestamp = 0L;
    qmf::Data event;

#ifdef HAVE_TIME
    timestamp = ::time(NULL);
#endif

    if (service == SRV_SERVICES) {
        // event = qmf::Data(_package.event_service_op);
        return;
    } else {
        event = qmf::Data(_package.event_resource_op);
    }

    event.setProperty("name", op->rsc);
    event.setProperty("action", op->action);
    event.setProperty("interval", op->interval);
    event.setProperty("rc", op->rc);
    event.setProperty("timestamp", timestamp);
    event.setProperty("sequence", op->sequence);

    if (service == SRV_RESOURCES) {
        event.setProperty("standard", op->standard);
        if(op->provider) {
            event.setProperty("provider", op->provider);
        }
        event.setProperty("agent", op->agent);
        event.setProperty("expected-rc", op->expected_rc);
    }

    if (userdata.length()) {
        event.setProperty("userdata", userdata);
    }

    getSession().raiseEvent(event);
}

int
SrvAgent::setup(qmf::AgentSession session)
{
#ifdef __linux__
    standards.push_back("ocf");
#endif
    standards.push_back("lsb");
#ifndef WIN32
    standards.push_back("windows");
#endif

    _package.configure(session);

    _services = qmf::Data(_package.data_Services);

    _services.setProperty("uuid", mh_uuid());
    _services.setProperty("hostname", mh_hostname());

    session.addData(_services, SERVICES_NAME);

    _resources = qmf::Data(_package.data_Resources);

    _resources.setProperty("uuid", mh_uuid());
    _resources.setProperty("hostname", mh_hostname());

    session.addData(_resources, RESOURCES_NAME);

    return 0;
}

gboolean
SrvAgent::invoke(qmf::AgentSession session, qmf::AgentEvent event,
                 gpointer user_data)
{
    if (event.getType() == qmf::AGENT_METHOD && event.hasDataAddr()) {
        if (event.getDataAddr().getName() == SERVICES_NAME) {
            return invoke_services(session, event, user_data);

        } else if (event.getDataAddr().getName() == RESOURCES_NAME) {
            return invoke_resources(session, event, user_data);

        } else {
            mh_err("Unknown agent: %s", event.getDataAddr().getName().c_str());
        }
    }

    mh_err("Unhandled message");
    return TRUE;
}

void
SrvAgent::action_async(enum service_id service, qmf::AgentSession& session,
                       qmf::AgentEvent& event, svc_action_t *op, bool has_rc)
{
    op->cb_data = new AsyncCB(this, service, session, event, has_rc);
    services_action_async(op, AsyncCB::mh_async_callback);
}

gboolean
SrvAgent::invoke_services(qmf::AgentSession session, qmf::AgentEvent event,
                          gpointer user_data)
{
    static const int default_timeout_ms = 60000;
    const std::string& methodName(event.getMethodName());
    if (event.getType() != qmf::AGENT_METHOD) {
        return TRUE;
    }

    qpid::types::Variant::Map& args = event.getArguments();

    if (methodName == "list") {
        _qtype::Variant::List s_list;
        GList *gIter = NULL;
        GList *services = services_list();

        for (gIter = services; gIter != NULL; gIter = gIter->next) {
            s_list.push_back((const char *) gIter->data);
        }

        g_list_free_full(services, free);

        event.addReturnArgument("agents", s_list);

    } else if (methodName == "enable" || methodName == "disable") {
        svc_action_t *op = services_action_create(
            args["name"].asString().c_str(), methodName.c_str(), 0,
            default_timeout_ms);

        if (!op) {
            session.raiseException(event, MH_INVALID_ARGS);
        } else {
            action_async(SRV_SERVICES, session, event, op, true);
        }

        return TRUE;

    } else if (methodName == "start"
               || methodName == "stop"
               || methodName == "status") {
        svc_action_t *op = services_action_create(
                args["name"].asString().c_str(), methodName.c_str(), 0, args["timeout"].asInt32());

        if (!op) {
            session.raiseException(event, MH_INVALID_ARGS);
        } else {
            action_async(SRV_SERVICES, session, event, op, true);
        }

        return TRUE;

    } else {
        session.raiseException(event, MH_NOT_IMPLEMENTED);
        return TRUE;
    }

    session.methodSuccess(event);
    return TRUE;
}

gboolean
SrvAgent::invoke_resources(qmf::AgentSession session, qmf::AgentEvent event,
                           gpointer user_data)
{
    const std::string& methodName(event.getMethodName());
    if (event.getType() != qmf::AGENT_METHOD) {
        return TRUE;
    }

    qpid::types::Variant::Map& args = event.getArguments();

    if (methodName == "list_standards") {
        event.addReturnArgument("standards", standards);

    } else if (methodName == "list_providers") {
        GList *gIter = NULL;
        GList *providers = NULL;
        _qtype::Variant::List p_list;

        providers = resources_list_providers(args["standard"].asString().c_str());
        for (gIter = providers; gIter != NULL; gIter = gIter->next) {
            p_list.push_back((const char *) gIter->data);
        }
        g_list_free_full(providers, free);

        event.addReturnArgument("providers", p_list);

    } else if (methodName == "list") {
        GList *gIter = NULL;
        GList *agents = NULL;
        std::string standard("ocf");
        std::string provider("heartbeat");
        _qtype::Variant::List t_list;

        if (args.count("standard")) {
            standard = args["standard"].asString();
        }

        if (args.count("provider")) {
            provider = args["provider"].asString();
        }

        agents = resources_list_agents(standard.c_str(), provider.c_str());
        for (gIter = agents; gIter != NULL; gIter = gIter->next) {
            t_list.push_back((const char *) gIter->data);
        }
        g_list_free_full(agents, free);

        event.addReturnArgument("agents", t_list);

    } else if (methodName == "invoke") {
        svc_action_t *op = NULL;
        bool valid_standard = false;
        _qtype::Variant::List::iterator iter;
        _qtype::Variant::Map map;

        if(args.count("parameters") == 1) {
            map = args["parameters"].asMap();
        }

        GHashTable *params = qmf_map_to_hash(map);

        int32_t interval = 0;
        int32_t timeout = 60000;
        std::string agent;
        std::string standard("ocf");
        std::string provider("heartbeat");

        for ( iter=standards.begin() ; iter != standards.end(); iter++ ) {
            if(args["standard"].asString() == (*iter).asString()) {
                valid_standard = true;
                break;
            }
        }

        if(valid_standard == false) {
            mh_err("%s is not a known resource standard", args["standard"].asString().c_str());
            session.raiseException(event, MH_NOT_IMPLEMENTED);
            return TRUE;
        }

        if (args.count("standard")) {
            standard = args["standard"].asString();
        }
        if (args.count("provider")) {
            provider = args["provider"].asString();
        }
        if (args.count("agent")) {
            agent = args["agent"].asString();
        } else {
            agent = args["name"].asString();
        }

        if(args.count("interval") > 0) {
            interval = args["interval"].asInt32();
        }
        if(args.count("timeout") > 0) {
            timeout = args["timeout"].asInt32();
        }

        op = resources_action_create(
            args["name"].asString().c_str(),
            standard.c_str(), provider.c_str(), agent.c_str(),
            args["action"].asString().c_str(),
            interval, timeout, params);

        if (!op) {
            session.raiseException(event, MH_INVALID_ARGS);
            return TRUE;
        }

        if(args.count("expected-rc") == 1) {
            op->expected_rc = args["expected-rc"].asInt32();
        }

        action_async(SRV_RESOURCES, session, event, op, true);
        return TRUE;

    } else if (methodName == "cancel") {
        services_action_cancel(
                args["name"].asString().c_str(),
                args["action"].asString().c_str(),
                args["interval"].asInt32());

    } else {
        session.raiseException(event, MH_NOT_IMPLEMENTED);
        return TRUE;
    }

    session.methodSuccess(event);
    return TRUE;
}
