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
    private:
	ManagementAgent* _agent;
	_qmf::Network* _management_object;
	
    public:
	int setup(ManagementAgent* agent);
	ManagementObject* GetManagementObject() const { return _management_object; }
	status_t ManagementMethod(uint32_t method, Args& arguments, string& text);
};

int
main(int argc, char **argv)
{
    NetAgent agent;
    int rc = agent.init(argc, argv);
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
NetAgent::setup(ManagementAgent* agent)
{
    this->_agent = agent;
    this->_management_object = new _qmf::Network(agent, this);
    this->_management_object->set_hostname(get_hostname());

    agent->addObject(this->_management_object);
    return 0;
}

Manageable::status_t
NetAgent::ManagementMethod(uint32_t method, Args& arguments, string& text)
{
    switch(method)
	{
	case _qmf::Network::METHOD_LIST:
	    {
                GList *interface_list = NULL;
                GList *plist = NULL;
                sigar_net_interface_config_t *ifconfig = NULL;
		
		_qmf::ArgsNetworkList& ioArgs = (_qmf::ArgsNetworkList&) arguments;
                interface_list = network_get_interfaces();
                for(plist = g_list_first(interface_list); plist; plist = g_list_next(plist)) {
                    ifconfig = (sigar_net_interface_config_t *)plist->data;
                    ioArgs.o_iface_map.push_back(ifconfig->name);
		}
	    }
	    return Manageable::STATUS_OK;

	case _qmf::Network::METHOD_START:
	    {
		_qmf::ArgsNetworkStart& ioArgs = (_qmf::ArgsNetworkStart&) arguments;
                ioArgs.o_status = interface_status(ioArgs.i_iface.c_str());
		if((ioArgs.o_status) == 1) {
		    network_start(ioArgs.i_iface.c_str());
		    ioArgs.o_status = interface_status(ioArgs.i_iface.c_str());
		}
	    }
	    return Manageable::STATUS_OK;
	    
	case _qmf::Network::METHOD_STOP:
	    {
		_qmf::ArgsNetworkStop& ioArgs = (_qmf::ArgsNetworkStop&) arguments;
                ioArgs.o_status = interface_status(ioArgs.i_iface.c_str());
		if(ioArgs.o_status == 0) {
		    network_stop(ioArgs.i_iface.c_str());
		    ioArgs.o_status = interface_status(ioArgs.i_iface.c_str());
		}
	    }
	    return Manageable::STATUS_OK;

	case _qmf::Network::METHOD_STATUS:
	    {
		_qmf::ArgsNetworkStatus& ioArgs = (_qmf::ArgsNetworkStatus&) arguments;
		ioArgs.o_status = interface_status(ioArgs.i_iface.c_str());

	    }
	    return Manageable::STATUS_OK;

	case _qmf::Network::METHOD_GET_IP_ADDRESS:
	    {
		_qmf::ArgsNetworkGet_ip_address& ioArgs = (_qmf::ArgsNetworkGet_ip_address&) arguments;

                ioArgs.o_ip = g_strdup((network_get_ip_address(ioArgs.i_iface.c_str())));
	    }
	    return Manageable::STATUS_OK;

	case _qmf::Network::METHOD_GET_MAC_ADDRESS:
	    {
                const char *mac_str;
		_qmf::ArgsNetworkGet_mac_address& ioArgs = (_qmf::ArgsNetworkGet_mac_address&) arguments;

                mac_str = network_get_mac_address(ioArgs.i_iface.c_str());
                ioArgs.o_mac = g_strdup(mac_str);
	    }
	    return Manageable::STATUS_OK;
	}
    return Manageable::STATUS_NOT_IMPLEMENTED;
}
