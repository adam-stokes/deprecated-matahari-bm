/* sysconfig-qmf.cpp - Copyright (C) 2011 Red Hat, Inc.
 * Written by Adam Stokes <astokes@fedoraproject.org>
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

/**
 * \file
 * \brief Sysconfig QMF Agent
 */

#ifndef WIN32
#include "config.h"
#endif

#include <qpid/agent/ManagementAgent.h>
#include "qmf/org/matahariproject/QmfPackage.h"
#include "matahari/agent.h"

extern "C" {
#include "matahari/logging.h"
#include "matahari/host.h"
#include "matahari/sysconfig.h"
};

class ConfigAgent : public MatahariAgent
{
private:
    qmf::org::matahariproject::PackageDefinition _package;
    qmf::Data _instance;
    static const char SYSCONFIG_NAME[];

public:
    virtual int setup(qmf::AgentSession session);
    virtual gboolean invoke(qmf::AgentSession session, qmf::AgentEvent event,
                            gpointer user_data);
};

const char ConfigAgent::SYSCONFIG_NAME[] = "Sysconfig";

class AsyncCB
{
public:
    AsyncCB(const std::string& _key, qmf::AgentEvent& _event,
            qmf::AgentSession& _session) :
                    key(_key), event(_event), session(_session) {}
    ~AsyncCB() {}

    static void result_cb(void *cb_data, int res);

private:
    std::string key;
    /** The method call that initiated this async action */
    qmf::AgentEvent event;
    /** The QMF session that initiated this async action */
    qmf::AgentSession session;
};

int
main(int argc, char **argv)
{
    ConfigAgent agent;
    int rc = agent.init(argc, argv, "Sysconfig");
    if (rc == 0) {
        mainloop_track_children(G_PRIORITY_DEFAULT);
        agent.run();
    }
    return rc;
}

int
ConfigAgent::setup(qmf::AgentSession session)
{
    _package.configure(session);
    _instance = qmf::Data(_package.data_Sysconfig);

    _instance.setProperty("hostname", mh_hostname());
    _instance.setProperty("uuid", mh_uuid());
    _instance.setProperty("is_postboot_configured", 0);

    session.addData(_instance, SYSCONFIG_NAME);
    return 0;
}

void
AsyncCB::result_cb(void *cb_data, int res)
{
    AsyncCB *action_data = static_cast<AsyncCB *>(cb_data);
    char *status;

    status = mh_sysconfig_is_configured(action_data->key.c_str());
    action_data->event.addReturnArgument("status", status ? status : "unknown");

    action_data->session.methodSuccess(action_data->event);

    free(status);
    delete action_data;
}

gboolean
ConfigAgent::invoke(qmf::AgentSession session, qmf::AgentEvent event, gpointer user_data)
{
    char *status = NULL;
    bool async = false;

    const std::string& methodName(event.getMethodName());
    if (event.getType() != qmf::AGENT_METHOD) {
        return TRUE;
    }

    qpid::types::Variant::Map& args = event.getArguments();

    if (methodName == "run_uri") {
        AsyncCB *action_data = new AsyncCB(args["key"].asString(), event, session);
        int res;

        res = mh_sysconfig_run_uri(args["uri"].asString().c_str(),
            args["flags"].asUint32(),
            args["scheme"].asString().c_str(),
            args["key"].asString().c_str(), AsyncCB::result_cb, action_data);

        if (res) {
            status = mh_sysconfig_is_configured(args["key"].asString().c_str());
            event.addReturnArgument("status", status ? status : "unknown");
            delete action_data;
        } else {
            async = true;
        }
    } else if (methodName == "run_string") {
        AsyncCB *action_data = new AsyncCB(args["key"].asString(), event, session);
        int res;

        res = mh_sysconfig_run_string(args["text"].asString().c_str(),
            args["flags"].asUint32(),
            args["scheme"].asString().c_str(),
            args["key"].asString().c_str(), AsyncCB::result_cb, action_data);

        if (res) {
            status = mh_sysconfig_is_configured(args["key"].asString().c_str());
            event.addReturnArgument("status", status ? status : "unknown");
            delete action_data;
        } else {
            async = true;
        }
    } else if (methodName == "query") {
        const char *data = NULL;
        data = mh_sysconfig_query(args["query"].asString().c_str(),
                                  args["flags"].asUint32(),
                                  args["scheme"].asString().c_str());
        event.addReturnArgument("data", data ? data : "unknown");
    } else if (methodName == "is_configured") {
        status = mh_sysconfig_is_configured(args["key"].asString().c_str());
        event.addReturnArgument("status", status ? status : "unknown");
    } else {
        session.raiseException(event, MH_NOT_IMPLEMENTED);
        goto bail;
    }

    free(status);

    if (!async) {
        session.methodSuccess(event);
    }

bail:
    return TRUE;
}
