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
#include <vector>
using namespace std;
using namespace qpid::console;

extern "C" {
#include <sigar.h>
#include "matahari/host.h"
}

class Listener : public ConsoleListener {
public:
    void brokerConnected(const Broker& broker) {
	cout << qpid::sys::now() << " broker joined" << endl;
    }

    void brokerDisconnected(const Broker& broker) {
	cout << qpid::sys::now() << " broker parted" << endl;
    }

    void event(Event& event) {
	cout << event << endl;
    }
};

int main(int argc, char** argv)
{
    qpid::client::ConnectionSettings settings;
    qpid::messaging::Connection amqp_connection;
    Listener listener;
    string servername = "localhost";
    string username = NULL;
    string password = NULL;
    string service = NULL;
    int serverport = 49000;

    settings.host = servername;
    settings.port = serverport;

    qpid::types::Variant::Map options;
    options["username"] = username;
    options["password"] = password;
    options["sasl-service"] = "qpid";
    options["sasl-mechanism"] = "GSSAPI";

    std::stringstream url;
    url << servername << ":" << serverport;
    amqp_connection = qpid::messaging::Connection(url.str(), options);
    amqp_connection.open();

    // Only receive events, cut out other chatter
    SessionManager::Settings sm_settings;
    sm_settings.rcvObjects = false;
    sm_settings.rcvHeartbeats = false;

    SessionManager sm(&listener, sm_settings);
    Broker* broker = sm.addBroker(settings);

    while (true) {
	qpid::sys::sleep(1);
    }
    sm.delBroker(broker);

    return 0;
}
