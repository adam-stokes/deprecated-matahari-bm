/* netagent.cpp - Copyright (C) 2010 Red Hat, Inc.
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

#ifndef WIN32
#include "config.h"
#endif

#include "matahari/agent.h"

#include "qmf/org/matahariproject/QmfPackage.h"

#include <qpid/agent/ManagementAgent.h>

extern "C" {
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <glib/gprintf.h>
#include "matahari/logging.h"
#include "matahari/network.h"
#include "matahari/host.h"
};

class NetAgent : public MatahariAgent
{
private:
    qmf::org::matahariproject::PackageDefinition _package;
    qmf::Data _instance;
    static const char NETWORK_NAME[];

public:
    virtual int setup(qmf::AgentSession session);
    virtual gboolean invoke(qmf::AgentSession session, qmf::AgentEvent event,
                            gpointer user_data);
};

const char NetAgent::NETWORK_NAME[] = "Network";

int
main(int argc, char **argv)
{
    NetAgent agent;
    int rc = agent.init(argc, argv, "Network");
    if (rc == 0) {
        agent.run();
    }
    return rc;
}

static int
interface_status(const char *iface)
{
    uint64_t flags = 0;

    if (iface == NULL) {
        return 3;
    }

    mh_network_status(iface, &flags);

    if (flags & MH_NETWORK_IF_UP) {
        return 0;
    }

    return 1; /* Inactive */
}

int
NetAgent::setup(qmf::AgentSession session)
{
    _package.configure(session);
    _instance = qmf::Data(_package.data_Network);

    _instance.setProperty("hostname", mh_hostname());
    _instance.setProperty("uuid", mh_uuid());

    session.addData(_instance, NETWORK_NAME);
    return 0;
}

gboolean
NetAgent::invoke(qmf::AgentSession session, qmf::AgentEvent event,
                 gpointer user_data)
{
    if (event.getType() != qmf::AGENT_METHOD) {
        return TRUE;
    }

    const std::string& methodName(event.getMethodName());
    qpid::types::Variant::Map& args = event.getArguments();

    if (methodName == "list") {
        GList *plist = NULL;
        GList *interface_list = NULL;

        _qtype::Variant::List s_list;

        interface_list = mh_network_get_interfaces();
        for (plist = g_list_first(interface_list); plist;
             plist = g_list_next(plist)) {
            struct mh_network_interface *iface =
                        static_cast<struct mh_network_interface *>(plist->data);
            s_list.push_back(mh_network_interface_get_name(iface));
        }
        g_list_free_full(interface_list, mh_network_interface_destroy);
        event.addReturnArgument("iface_map", s_list);
    } else if (methodName == "start") {
        int rc = interface_status(
                args["iface"].asString().c_str());

        if (rc == 1) {
            mh_network_start(args["iface"].asString().c_str());
            rc = interface_status(
                    args["iface"].asString().c_str());
        }
        event.addReturnArgument("status", rc);
    } else if (methodName == "stop") {
        int rc = interface_status(
                args["iface"].asString().c_str());
        if (rc == 0) {
            mh_network_stop(args["iface"].asString().c_str());
            rc = interface_status(
                    args["iface"].asString().c_str());
        }
        event.addReturnArgument("status", rc);
    } else if (methodName == "status") {
        event.addReturnArgument("status", interface_status(
                args["iface"].asString().c_str()));
    } else if (methodName == "get_ip_address") {
        char buf[64];
        event.addReturnArgument("ip", mh_network_get_ip_address(
                args["iface"].asString().c_str(),
                buf, sizeof(buf)));
    } else if (methodName == "get_mac_address") {
        char buf[32];
        event.addReturnArgument("mac", mh_network_get_mac_address(
                args["iface"].asString().c_str(),
                buf, sizeof(buf)));
    } else {
        session.raiseException(event, MH_NOT_IMPLEMENTED);
        goto bail;
    }

    session.methodSuccess(event);

bail:
    return TRUE;
}
