/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 */

#ifndef WIN32
#include "config.h"
#endif

#include <qpid/messaging/Connection.h>
#include <qpid/messaging/Duration.h>
#include <qmf/ConsoleSession.h>
#include <qmf/ConsoleEvent.h>
#include <qmf/Agent.h>
#include <qmf/Data.h>
#include <qmf/DataAddr.h>
#include <qmf/Query.h>
#include <qpid/types/Variant.h>

#include <matahari/agent.h>

#include <string>
#include <iostream>
#include <sstream>

extern "C" {
#include "matahari/logging.h"
}

using namespace std;
using namespace qmf;
using qpid::types::Variant;
using qpid::messaging::Duration;

static int
resource_arg(int code, const char *name, const char *arg, void *userdata)
{
    qpid::types::Variant::Map *options = static_cast<qpid::types::Variant::Map*>(userdata);
    if(options->count(name) > 0) {
	options->erase(name);
    }
    
    cout << name << "=" << arg << endl;
    if(strcmp(name, "timeout") == 0 || strcmp(name, "interval") == 0) {
	uint32_t number = 1000 * atoi(arg);
	options->insert(std::pair<std::string, qpid::types::Variant>(name, number));

    } else {
	options->insert(std::pair<std::string, qpid::types::Variant>(name, arg));
    }

    return 0;
}

int main(int argc, char** argv)
{
    qpid::types::Variant::Map options;
    qpid::types::Variant::Map callOptions;
    string sessionOptions;
    ConsoleEvent event;
    Agent agent;

    options["api"] = "Resources";
    options["host-dns"] = mh_hostname();
    options["standard"] = "ocf";
    options["provider"] = "heartbeat";
    options["interval"] = 0;
    options["timeout"] = 60000;
    options["parameters"] = qpid::types::Variant::Map();

    mh_log_init("service-console", LOG_TRACE, TRUE);

    mh_add_option('A', required_argument, "api", "Resources|Services", &options, resource_arg);
    mh_add_option('H', required_argument, "host-dns", "Host DNS name", &options, resource_arg);
    mh_add_option('U', required_argument, "host-uuid", "Host UUID", &options, resource_arg);

    mh_add_option('n', required_argument, "name", "Name of a resource", &options, resource_arg);
    mh_add_option('s', required_argument, "standard", "lsb|ocf|windows (Resources API only)", &options, resource_arg);
    mh_add_option('S', required_argument, "provider", " (Resources API only)", &options, resource_arg);
    mh_add_option('a', required_argument, "agent", " (Resources API only)", &options, resource_arg);

    mh_add_option('a', required_argument, "action", "Action to perform", &options, resource_arg);
    mh_add_option('i', required_argument, "interval", "(Resources API only)", &options, resource_arg);
    mh_add_option('t', required_argument, "timeout", "Time to wait, in seconds, for the action to complete", &options, resource_arg);

    mh_add_option('o', required_argument, "option", "Option to pass to the resource script (Resources API only)", NULL, NULL);

    qpid::types::Variant::Map amqpOptions = mh_parse_options("service-console", argc, argv, options);

    /* Re-initialize logging now that we've completed option processing */
    mh_log_init("service-console", mh_log_level, mh_log_level > LOG_INFO);

    qpid::messaging::Connection connection = mh_connect(options, amqpOptions, TRUE);
    ConsoleSession session(connection, sessionOptions);
    std::stringstream filter;

    filter << "[and";

    filter << ", [eq, _vendor, [quote, 'matahariproject.org']]";
    filter << ", [eq, _product, [quote, 'service']]";
    if (options.count("action") && options.count("host-uuid")) {
	filter << ", [eq, hostname, [quote, " << options["host-uuid"] << "]]";

    } else if (options.count("action") && options.count("host-dns")) {
	/* Restrict further, to a single host */
	filter << ", [eq, hostname, [quote, " << options["host-dns"] << "]]";
    }
    filter << "]";

    /* Only interested in agents under matahariproject.org vendor
     * Set before opening the session to avoid unwanted events
     */
    session.setAgentFilter(filter.str());
    session.open();

    if (options.count("action")) {
	uint32_t lpc = 1;
	const char *action = options["action"].asString().c_str();
	cout << "Building " << options["api"] << " options" << endl;
	if(options["api"] == "Services") {
	    if(options["action"] == "list") {
	    } else if(options["action"] == "enable") {
		callOptions["name"] = options["name"].asString();
	    } else if(options["action"] == "disable") {
		callOptions["name"] = options["name"].asString();
	    } else {
		callOptions["name"] = options["name"].asString();
		callOptions["timeout"] = options["timeout"].asUint32();
	    }

	} else {
	    if(strstr(action, "list") != NULL) {
		callOptions["standard"] = options["standard"].asString();
		callOptions["provider"] = options["provider"].asString();

	    } else {
		action = "invoke";
		callOptions = options;
		callOptions.erase("api");
		callOptions.erase("reconnect");
		callOptions.erase("host-dns");
		callOptions.erase("host-uuid");
		callOptions.erase("protocol");
	    }
	}
	
	while(session.getAgentCount() == 0) {
	    g_usleep(1000);
	}

	for(lpc = 0; lpc < session.getAgentCount(); lpc++) {
	    agent = session.getAgent(lpc);
	    cout << agent.getName() << endl;

	    // event = agent.query("class: Resources, package: 'org.matahariproject', where: [eq, hostname, [quote, f15.beekhof.net]]}");
	    DataAddr agent_data(options["api"], agent.getName(), 0);

	    cout << callOptions << endl;
	    event = agent.callMethod(action, callOptions, agent_data);
	    if(event.getType() != CONSOLE_METHOD_RESPONSE) {
		uint32_t llpc = 0;
		cout << "Call failed: " << event.getType() << endl;
		for ( ; llpc < event.getDataCount(); llpc++) {
		    cout << "Error data: " << event.getData(llpc).getProperties() << endl;
		}
		
	    } else {
		cout << event.getArguments() << endl;
	    }
	    
//	    event = agent.query("{class: Resources}");
//	    cout << event.getAgent().getName() << endl;
	}	
    }
    
    while (options.count("action") == 0) {
	if(session.nextEvent(event)) {
	    switch(event.getType()) {
		case CONSOLE_AGENT_ADD:
		    // if(event.getAgent().getProduct() == "service") {  <-- implied by our filter
		    {
			cout << "Agent " << event.getAgent().getName() << " on " << event.getAgent().getAttribute("hostname") << " is now available supporting the following agents:";
		    
			/* The rest is mostly to show we can */
			agent = event.getAgent();
			DataAddr agent_data(options["api"], agent.getName(), 0);
			// event = agent.callMethod("list", callOptions, agent_data);
			
			if (options["api"] == "Resources") {
			    callOptions["standard"] = options["standard"];
			    callOptions["provider"] = options["provider"];
			}
			
			event = agent.callMethod("list", callOptions, agent_data);
			if(event.getType() != CONSOLE_METHOD_RESPONSE) {
			    uint32_t llpc = 0;
			    cout << "Call failed: " << event.getType() << endl;
			    for ( ; llpc < event.getDataCount(); llpc++) {
				cout << "Error data: " << event.getData(llpc).getProperties() << endl;
			    }
			    
			} else {
			    int count = 0;
			    qpid::types::Variant::Map output = event.getArguments();
			    qpid::types::Variant::List agents = output["agents"].asList();
			    qpid::types::Variant::List::iterator iter = agents.begin();
			    while(iter != agents.end()) {
				if(count % 12 == 0) {
				    cout << endl << "    ";
				}
				cout << *iter << " ";
				count++;
				iter++;
			    }
			    cout << endl;
			}
		    }
		    break;
		case CONSOLE_AGENT_DEL:
		    cout << "Agent " << event.getAgent().getName() << " is no longer available" << endl;
		    break;
		    
		case CONSOLE_EVENT:
		    {
			uint32_t llpc = 0;
			cout << "Event from agent " << event.getAgent().getName() << ": " << endl;
			for ( ; llpc < event.getDataCount(); llpc++) {
			    cout << "Event data: " << event.getData(llpc).getProperties() << endl;
			}
		    }
		    break;
		default: {
		    cout << "Unknown event "<< event.getType() <<" from agent " << event.getAgent().getName() << ": " << event.getArguments() << endl;
		}
	    }
	}
    }
    return 0;
}
