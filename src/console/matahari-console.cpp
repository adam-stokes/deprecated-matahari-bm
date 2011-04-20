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
#include <qmf/Data.h>
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
    string url("localhost:49000"); 
    string connectionOptions;
    string sessionOptions;

    qpid::messaging::Connection connection(url, connectionOptions);
    connection.open();

    ConsoleSession session(connection, sessionOptions);
    session.open();

    while (true) {
        ConsoleEvent event;
        if(session.nextEvent(event)) {
            if(event.getType() == CONSOLE_EVENT) {
                const Data& data(event.getData(0));
                cout << "Event: timestamp=" << event.getTimestamp() << " severity=" <<
                    event.getSeverity() << " content=" << data.getProperties() << endl;
            }
        }
    }
    return 0;
}
