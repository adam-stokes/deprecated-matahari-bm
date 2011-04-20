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

#include "matahari/mh_agent.h"

#include "qmf/org/matahariproject/Network.h"
#include "qmf/org/matahariproject/ArgsNetworkList.h"
#include "qmf/org/matahariproject/ArgsNetworkStop.h"
#include "qmf/org/matahariproject/ArgsNetworkStart.h"
#include "qmf/org/matahariproject/ArgsNetworkStatus.h"
#include "qmf/org/matahariproject/ArgsNetworkGet_ip_address.h"
#include "qmf/org/matahariproject/ArgsNetworkGet_mac_address.h"

#include <qpid/agent/ManagementAgent.h>

extern "C" {
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <glib/gprintf.h>
#include "matahari/logging.h"
#include "matahari/network.h"
#include "matahari/host.h"
#include <sigar.h>
#include <sigar_format.h>
};

class NetAgent : public MatahariAgent
{
public:
    virtual int setup(qmf::AgentSession session);
    virtual gboolean invoke(qmf::AgentSession session, qmf::AgentEvent event,
                            gpointer user_data);
};

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

static int interface_status(const char *iface)
{
    uint64_t flags = 0;
    if(iface == NULL)
       return 3;

    network_status(iface, &flags);

    if(flags & SIGAR_IFF_UP) {
       return 0;
    }
    return 1; /* Inactive */
}

int
NetAgent::setup(qmf::AgentSession session)
{
    _instance = qmf::Data(_package.data_Network);

    _instance.setProperty("hostname", matahari_hostname());
    _instance.setProperty("uuid", matahari_uuid());

    _agent_session.addData(_instance);
    return 0;
}

gboolean
NetAgent::invoke(qmf::AgentSession session, qmf::AgentEvent event, gpointer user_data)
{
    if (event.getType() != qmf::AGENT_METHOD) {
        session.methodSuccess(event);
        return TRUE;
    }

    const std::string& methodName(event.getMethodName());

    if (methodName == "list") {
        GList *plist = NULL;
        GList *interface_list = NULL;

        _qtype::Variant::List s_list;
        sigar_net_interface_config_t *ifconfig = NULL;

        interface_list = network_get_interfaces();
        for (plist = g_list_first(interface_list); plist; plist = g_list_next(plist)) {
            ifconfig = (sigar_net_interface_config_t *)plist->data;
            s_list.push_back(ifconfig->name);
        }
        event.addReturnArgument("iface_map", s_list);
    } else if (methodName == "start") {
        int rc = interface_status(event.getArguments()["iface"].asString().c_str());

        if (rc == 1) {
            network_start(event.getArguments()["iface"].asString().c_str());
            rc = interface_status(event.getArguments()["iface"].asString().c_str());
        }
        event.addReturnArgument("status", rc);
    } else if (methodName == "stop") {
        int rc = interface_status(event.getArguments()["iface"].asString().c_str());
        if (rc == 0) {
            network_stop(event.getArguments()["iface"].asString().c_str());
            rc = interface_status(event.getArguments()["iface"].asString().c_str());
        }
        event.addReturnArgument("status", rc);
    } else if (methodName == "status") {
        event.addReturnArgument("status", interface_status(event.getArguments()["iface"].asString().c_str()));
    } else if (methodName == "get_ip_address") {
        event.addReturnArgument("ip", network_get_ip_address(event.getArguments()["iface"].asString().c_str()));
    } else if (methodName == "get_mac_address") {
        event.addReturnArgument("mac", network_get_mac_address(event.getArguments()["iface"].asString().c_str()));
    } else {
        session.raiseException(event, MH_NOT_IMPLEMENTED);
        goto bail;
    }

    session.methodSuccess(event);

bail:
    return TRUE;
}
