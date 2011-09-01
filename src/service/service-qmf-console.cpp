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
call_arg(int code, const char *name, const char *arg, void *userdata)
{
    qpid::types::Variant::Map *options = static_cast<qpid::types::Variant::Map*>(userdata);
    mh_debug("Found call option: '%s' = '%s'", name, arg);
    if(strcmp(name, "timeout") == 0 || strcmp(name, "interval") == 0) {
        uint32_t number = atoi(arg);
        (*options)[name] = number;
    } else {
        (*options)[name] = arg;
    }

    return 0;
}

static int
agent_arg(int code, const char *name, const char *arg, void *userdata)
{
    qpid::types::Variant::Map *options = static_cast<qpid::types::Variant::Map*>(userdata);
    string tokenize = arg;
    string delimiters = " \t=";
    string::size_type start = tokenize.find_first_not_of(delimiters, 0);
    string::size_type delim = tokenize.find_first_of(delimiters, start);

    if (string::npos != delim || string::npos != start) {
        string opt_name = tokenize.substr(start, delim - start);

        start = tokenize.find_first_not_of(delimiters, delim);
        string opt_value = tokenize.substr(start);

        mh_debug("Found agent option: '%s' = '%s'", opt_name.c_str(), opt_value.c_str());
        (*options)[opt_name] = opt_value;
    }
    return 0;
}

int main(int argc, char** argv)
{
    qpid::types::Variant::Map options;
    qpid::types::Variant::Map core_options;
    qpid::types::Variant::Map agent_options;
    string sessionOptions;
    ConsoleEvent event;
    Agent agent;

    core_options["api-type"] = "Resources";
    core_options["host-dns"] = mh_hostname();
    options["standard"] = "ocf";
    options["provider"] = "heartbeat";
    options["interval"] = 0;
    options["timeout"] = 60000;

    mh_log_init("service-cli", LOG_INFO, TRUE);

    mh_add_option('T', required_argument, "api-type", "Resources|Services", &core_options, call_arg);
    mh_add_option('H', required_argument, "host-dns", "Host DNS name", &core_options, call_arg);
    mh_add_option('U', required_argument, "host-uuid", "Host UUID", &core_options, call_arg);

    mh_add_option('N', required_argument, "name", "Name of a resource", &options, call_arg);
    mh_add_option('S', required_argument, "standard", "lsb|ocf|windows (Resources API only)", &options, call_arg);
    mh_add_option('J', required_argument, "provider", "(Resources API only)", &options, call_arg);
    mh_add_option('A', required_argument, "agent", "(Resources API only)", &options, call_arg);

    mh_add_option('a', required_argument, "action", "Action to perform", &options, call_arg);
    mh_add_option('i', required_argument, "interval", "(Resources API only) Time, in milliseconds, in between recurring executions of this action.", &options, call_arg);
    mh_add_option('t', required_argument, "timeout", "Time to wait, in milliseconds, for the action to complete", &options, call_arg);

    mh_add_option('o', required_argument, "option", "Option to pass to the resource script (Resources API only)", &agent_options, agent_arg);

    qpid::types::Variant::Map amqpOptions = mh_parse_options("qmf-service-cli", argc, argv, core_options);
    options["parameters"] = agent_options;

    /* Re-initialize logging now that we've completed option processing */
    mh_log_init("service-cli", mh_log_level, mh_log_level > LOG_INFO);

    qpid::messaging::Connection connection = mh_connect(core_options, amqpOptions, TRUE);
    ConsoleSession session(connection, sessionOptions);
    std::stringstream filter;

    filter << "[and";

    filter << ", [eq, _vendor, [quote, 'matahariproject.org']]";
    filter << ", [eq, _product, [quote, 'service']]";
    if (options.count("action") && core_options.count("host-uuid")) {
        filter << ", [eq, hostname, [quote, " << core_options["host-uuid"] << "]]";

    } else if (options.count("action") && core_options.count("host-dns")) {
        /* Restrict further, to a single host */
        filter << ", [eq, hostname, [quote, " << core_options["host-dns"] << "]]";
    }
    filter << "]";

    /* Only interested in agents under matahariproject.org vendor
     * Set before opening the session to avoid unwanted events
     */
    session.setAgentFilter(filter.str());
    session.open();

    if (options.count("action")) {
        uint32_t lpc = 1;
        qpid::types::Variant::Map call_options;
        std::string action = options["action"].asString();
        mh_debug("Building %s options...", core_options["api-type"].asString().c_str());
        if(core_options["api-type"] == "Services") {
            if(options["action"] == "list") {
            } else if(options["action"] == "enable") {
                call_options["name"] = options["name"].asString();
            } else if(options["action"] == "disable") {
                call_options["name"] = options["name"].asString();
            } else {
                call_options["name"] = options["name"].asString();
                call_options["timeout"] = options["timeout"].asUint32();
            }

        } else {
            if(strstr(action.c_str(), "list") != NULL) {
                call_options["standard"] = options["standard"].asString();
                call_options["provider"] = options["provider"].asString();

            } else {
                action = "invoke";
                call_options = options;
            }
        }

        while(session.getAgentCount() == 0) {
            g_usleep(1000);
        }

        for(lpc = 0; lpc < session.getAgentCount(); lpc++) {
            agent = session.getAgent(lpc);
            cout << agent.getName() << endl;

            // event = agent.query("class: Resources, package: 'org.matahariproject', where: [eq, hostname, [quote, f15.beekhof.net]]}");
            DataAddr agent_data(core_options["api-type"], agent.getName(), 0);

            cout << "Call options: " << call_options << endl;
            event = agent.callMethod(action, call_options, agent_data);
            if(event.getType() != CONSOLE_METHOD_RESPONSE) {
                uint32_t llpc = 0;
                cout << "Call failed: " << event.getType() << endl;
                for ( ; llpc < event.getDataCount(); llpc++) {
                    cout << "Error data: " << event.getData(llpc).getProperties() << endl;
                }

            } else {
                cout << "Call returned: " << event.getArguments() << endl;
            }

//            event = agent.query("{class: Resources}");
//            cout << event.getAgent().getName() << endl;
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
                        qpid::types::Variant::Map call_options;
                        DataAddr agent_data(core_options["api-type"], agent.getName(), 0);
                        // event = agent.callMethod("list", call_options, agent_data);

                        if (core_options["api-type"] == "Resources") {
                            call_options["standard"] = options["standard"];
                            call_options["provider"] = options["provider"];
                        }

                        event = agent.callMethod("list", call_options, agent_data);
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
