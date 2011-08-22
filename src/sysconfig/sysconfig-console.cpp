/* config-console.cpp - Copyright (c) 2011 Red Hat, Inc.
 * Written by Adam Stokes <astokes@fedoraproject.org>
 *
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
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

#include "matahari/agent.h"

#include <string>
#include <iostream>
#include <sstream>

extern "C" {
#include "matahari/logging.h"
#include "matahari/sysconfig.h"
}

using namespace std;
using namespace qmf;
using qpid::types::Variant;
using qpid::messaging::Duration;

int main(int argc, char **argv)
{
    qpid::types::Variant::Map options;
    qpid::types::Variant::Map callOptions;
    qpid::messaging::Connection connection;
    string sessionOptions;
    ConsoleEvent event;
    Agent agent;

    mh_log_init("sysconfig-console", LOG_TRACE, TRUE);

    mh_add_option('U', required_argument, "uri", "URI of configuration", &options, NULL);
    mh_add_option('d', no_argument, "daemon", "run as a daemon", NULL, mh_should_daemonize);

    qpid::types::Variant::Map amqp_options = mh_parse_options("sysconfig-console", argc, argv, options);

    callOptions["uri"] = options["uri"];

    mh_log_init("sysconfig-console", mh_log_level, mh_log_level > LOG_INFO);

    connection = mh_connect(options, amqp_options, TRUE);
    connection.open();

    ConsoleSession session(connection, sessionOptions);
    // Only filter connecting agents under matahariproject.org vendor and Config product
    session.setAgentFilter("[and, [eq, _vendor, [quote, 'matahariproject.org']], [eq, _product, [quote, 'Sysconfig']]]");
    session.open();

    while(session.getAgentCount() == 0) {
        g_usleep(1000);
    }

    while (true) {
        if(session.nextEvent(event)) {
            switch(event.getType()) {
                case CONSOLE_AGENT_ADD:
                    {
                        agent = event.getAgent();
                        DataAddr agent_event_addr("Sysconfig", agent.getName(), 0);
                        ConsoleEvent result(agent.callMethod("run_uri",
                                                              callOptions,
                                                              agent_event_addr));
                    }
                    break;
                default:
                    break;
            }
        }
    }
    return 0;
}
