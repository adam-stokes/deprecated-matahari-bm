/* netagent.cpp - Copyright (C) 2010 Red Hat, Inc.
 * Written by Adam Stokes <astokes@fedoraproject.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.  A copy of the GNU General Public License is
 * also available at http://www.gnu.org/copyleft/gpl.html.
 */

#ifndef WIN32
#include "config.h"
#endif

#include "netagent.h"
#include <qpid/agent/ManagementAgent.h>
#include "qmf/com/redhat/matahari/ArgsNetworkList.h"
#include "qmf/com/redhat/matahari/ArgsNetworkStop.h"
#include "qmf/com/redhat/matahari/ArgsNetworkStart.h"
#include "qmf/com/redhat/matahari/ArgsNetworkStatus.h"
#include "qmf/com/redhat/matahari/ArgsNetworkDescribe.h"
#include "qmf/com/redhat/matahari/ArgsNetworkDestroy.h"
#include "host.h" 

extern "C" { 
#include <netcf.h> 
#include <string.h>
};

struct netcf *ncf;
static int interface_status(struct netcf_if *nif) 
{
    unsigned int flags = 0;
    if(nif == NULL) {
	return 3;
	
    } else if(ncf_if_status(nif, &flags) < 0) {
	return 4;

    } else if(flags & NETCF_IFACE_ACTIVE) {
	return 0;
    }
    return 1; /* Inactive */
}

NetAgent::NetAgent(ManagementAgent* agent)
{
    if(	ncf == NULL) {
	return;
    }

    this->_agent = agent;
    this->_management_object = new _qmf::Network(agent, this);
    this->_management_object->set_hostname(host_get_hostname());

    agent->addObject(this->_management_object);
}

NetAgent::~NetAgent() { }

int
NetAgent::setup(ManagementAgent* agent)
{
    if (ncf_init(&ncf, NULL) < 0) {
	return -1;
    }
    return 0;
}

Manageable::status_t
NetAgent::ManagementMethod(uint32_t method, Args& arguments, string& text)
{
    struct netcf_if *nif;

    if(	ncf == NULL) {
	return Manageable::STATUS_NOT_IMPLEMENTED;
    }
    
    switch(method)
	{
	case _qmf::Network::METHOD_LIST:
	    {
		_qmf::ArgsNetworkList& ioArgs = (_qmf::ArgsNetworkList&) arguments;
		uint32_t lpc = 0;
		char **iface_list = NULL;
		ioArgs.o_max = ncf_num_of_interfaces(ncf, NETCF_IFACE_ACTIVE|NETCF_IFACE_INACTIVE);
		
		iface_list = (char**)calloc(ioArgs.o_max+1, sizeof(char*));
		if(ncf_list_interfaces(ncf, ioArgs.o_max, iface_list, NETCF_IFACE_ACTIVE|NETCF_IFACE_INACTIVE) < 0) {
		    ioArgs.o_max = 0;
		}
		
		for(lpc = 0; lpc < ioArgs.o_max; lpc++) {
		    nif = ncf_lookup_by_name(ncf, iface_list[lpc]);
		    ioArgs.o_iface_map.setString(iface_list[lpc], ncf_if_mac_string(nif));
		}
	    }
	    return Manageable::STATUS_OK;

	case _qmf::Network::METHOD_START:
	    {
		_qmf::ArgsNetworkStart& ioArgs = (_qmf::ArgsNetworkStart&) arguments;
		nif = ncf_lookup_by_name(ncf, ioArgs.i_iface.c_str());
		ioArgs.o_status = interface_status(nif);
		if(ioArgs.o_status == 1) {
		    ncf_if_up(nif);
		    ioArgs.o_status = interface_status(nif);
		}
	    }
	    return Manageable::STATUS_OK;
	    
	case _qmf::Network::METHOD_STOP:
	    {
		_qmf::ArgsNetworkStop& ioArgs = (_qmf::ArgsNetworkStop&) arguments;
		nif = ncf_lookup_by_name(ncf, ioArgs.i_iface.c_str());
		ioArgs.o_status = interface_status(nif);
		if(ioArgs.o_status == 0) {
		    ncf_if_down(nif);
		    ioArgs.o_status = interface_status(nif);
		}
	    }
	    return Manageable::STATUS_OK;

	case _qmf::Network::METHOD_STATUS:
	    {
		_qmf::ArgsNetworkStatus& ioArgs = (_qmf::ArgsNetworkStatus&) arguments;
		nif = ncf_lookup_by_name(ncf, ioArgs.i_iface.c_str());
		ioArgs.o_status = interface_status(nif);
	    }
	    return Manageable::STATUS_OK;

	case _qmf::Network::METHOD_DESCRIBE:
	    {
		_qmf::ArgsNetworkDescribe& ioArgs = (_qmf::ArgsNetworkDescribe&) arguments;
		nif = ncf_lookup_by_name(ncf, ioArgs.i_iface.c_str());
		if(nif != NULL) {
		    ioArgs.o_xml = ncf_if_xml_desc(nif);
		}
	    }
	    return Manageable::STATUS_OK;

	case _qmf::Network::METHOD_DESTROY:
	    {
		_qmf::ArgsNetworkDestroy& ioArgs = (_qmf::ArgsNetworkDestroy&) arguments;
		nif = ncf_lookup_by_name(ncf, ioArgs.i_iface.c_str());
		if(nif != NULL) {
		    ncf_if_undefine(nif);
		}
	    }
	    return Manageable::STATUS_OK;
	}
    return Manageable::STATUS_NOT_IMPLEMENTED;
}
