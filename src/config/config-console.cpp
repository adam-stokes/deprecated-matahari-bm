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

#include <string>
#include <iostream>

using namespace std;
using namespace qmf;
using qpid::types::Variant;
using qpid::messaging::Duration;

int main(int argc, char** argv)
{
    string brokerUrl("amqp:tcp:127.0.0.1:49000");
    qpid::types::Variant::Map connectionOptions;
    qpid::types::Variant::Map dataProperties;
    string sessionOptions;
    
    connectionOptions["reconnect"] = true;
    
    dataProperties["uri"] = argv[1] ? argv[1] : "http://localhost/config/uuid";

    qpid::messaging::Connection connection(brokerUrl, connectionOptions);
    connection.open();

    ConsoleSession session(connection, sessionOptions);
    session.open();

    // Only filter connecting agents under matahariproject.org vendor
    session.setAgentFilter("[eq, _vendor, [quote, 'matahariproject.org']]]");
    Agent agent;
    while (true) {
        ConsoleEvent event;
        if(session.nextEvent(event)) {
            switch(event.getType()) {
                case CONSOLE_AGENT_ADD:
                    {
                        if (session.getAgentCount() == 1) {
                            agent = session.getAgent(0);
                            DataAddr agent_event_addr("config", agent.getName(), 0);
                            cout << "callMethod:configure : " << dataProperties << endl;
                            ConsoleEvent result(agent.callMethod("configure", 
                                                                  dataProperties,
                                                                  agent_event_addr));
                        }
                    }
                    break;
                default: {}
            }
        }
    }
    return 0;
}
