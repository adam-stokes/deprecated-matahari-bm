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

int
main(int argc, char **argv)
{
    ConfigAgent agent;
    int rc = agent.init(argc, argv, "Sysconfig");
    if (rc == 0) {
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

gboolean
ConfigAgent::invoke(qmf::AgentSession session, qmf::AgentEvent event, gpointer user_data)
{
    char *status = NULL;

    const std::string& methodName(event.getMethodName());
    if (event.getType() != qmf::AGENT_METHOD) {
        return TRUE;
    }

    qpid::types::Variant::Map& args = event.getArguments();

    if (methodName == "run_uri") {
        mh_sysconfig_run_uri(args["uri"].asString().c_str(),
            args["flags"].asUint32(),
            args["scheme"].asString().c_str(),
            args["key"].asString().c_str());
        status = mh_sysconfig_is_configured(args["key"].asString().c_str());
        event.addReturnArgument("status", status ? status : "unknown");
    } else if (methodName == "run_string") {
        mh_sysconfig_run_string(args["text"].asString().c_str(),
            args["flags"].asUint32(),
            args["scheme"].asString().c_str(),
            args["key"].asString().c_str());
        status = mh_sysconfig_is_configured(args["key"].asString().c_str());
        event.addReturnArgument("status", status ? status : "unknown");
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
    session.methodSuccess(event);

bail:
    return TRUE;
}
