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

#include <qpid/console/ConsoleListener.h>
#include <qpid/console/SessionManager.h>
#include <qpid/messaging/Connection.h>
#include <qpid/client/ConnectionSettings.h>
#include <qpid/sys/Time.h>

#include "qmf/org/matahariproject/QmfPackage.h"

#include <sstream>
using namespace std;
using namespace qpid::console;

class Listener: public ConsoleListener {
    void event(Event& event) {
		cout << "hai im watching broker: " << event.getBroker()->getUrl() << endl;
    }

	void methodResponse(Broker& broker, uint32_t seq, MethodResponse& methodResponse) {
		cout << "response" << endl;
	}
};

int main(int argc, char** argv)
{
    Listener listener;

    qpid::client::ConnectionSettings settings;
    settings.host = "localhost";
    settings.port = 49000;

    qpid::types::Variant::Map options;
    options["sasl-service"] = "";
    options["sasl-mechanism"] = "";

    qpid::messaging::Connection amqp_connection;
    amqp_connection = qpid::messaging::Connection("localhost:49000", options);
    amqp_connection.open();

    // Only receive events, cut out other chatter
    SessionManager::Settings smsettings;

    SessionManager sm(&listener, smsettings);
    smsettings.rcvObjects = false;
    smsettings.rcvHeartbeats = false;
    Broker* broker = sm.addBroker(settings);

    while (true) {
		qpid::sys::sleep(1);
    }
    sm.delBroker(broker);

    return 0;
}
