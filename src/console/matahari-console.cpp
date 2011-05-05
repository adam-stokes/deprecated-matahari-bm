/* matahari-console.cpp - Copyright (c) 2011 Red Hat, Inc.
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

#include "qmf/org/matahariproject/QmfPackage.h"

#include <string>
#include <iostream>

using namespace std;
using namespace qmf;
using qpid::types::Variant;
using qpid::messaging::Duration;

int main(int argc, char** argv)
{
    string url("amqp:tcp:127.0.0.1:49000");
    const string methodName("configure");
    qpid::types::Variant::Map connectionOptions;
    qpid::types::Variant::Map dataProperties;
    string sessionOptions;
    
    connectionOptions["reconnect"] = false;

    qpid::messaging::Connection connection(url, connectionOptions);
    connection.open();

    ConsoleSession session(connection, sessionOptions);
    session.open();

    Agent agent;
    while (true) {
        ConsoleEvent event;
        if(session.nextEvent(event)) {
            switch(event.getType()) {
                case CONSOLE_AGENT_ADD:
                    {
                        cout << "Agent added : " << event.getAgent().getName() << endl;
                        agent = event.getAgent();
                        ConsoleEvent result(agent.query("{class:Config, package:'org.matahariproject'}"));
                        if(result.getDataCount() == 1) {
                            const Data& data(result.getData(0));
                            dataProperties = data.getProperties();
                            cout << "data : " << dataProperties << endl;
                            ConsoleEvent result(agent.callMethod(methodName, dataProperties, data.getAddr(), qpid::messaging::Duration(4000)));
                        } else {
                            cout << "No objects in query" << endl;
                        }
                    }
                    break;
                default: {}
            }
        }
    }
    return 0;
}
