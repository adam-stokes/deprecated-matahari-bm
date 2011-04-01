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

#include "qpid/console/ConsoleListener.h"
#include "qpid/console/SessionManager.h"
#include "qpid/sys/Time.h"
#include <signal.h>

using namespace std;
using namespace qpid::console;

/* 
 * listener class for responding to the events raised by our
 * matahari agents
 */

class Listener : public ConsoleListener {
public:
  ~Listener() {}
  void brokerConnected(const Broker& broker) {
    cout << "broker connected: " << broker << endl;
  }

  void brokerDisconnected(const Broker& broker) {
    cout << "broker disconnected: " << broker << endl;
  }

  void event(Event& event) {
    cout << "event raised: " << event << endl;
  }

};

int main(int argc, char** argv)
{
  Listener listener;
  qpid::client::ConnectionSettings settings;

  SessionManager::Settings smSettings;
  smSettings.rcvObjects = false;
  smSettings.rcvHeartbeats = false;
  SessionManager sm(&listener, smSettings);
  cout << "Adding broker" << endl;
  Broker* broker = sm.addBroker(settings);
  while(true)
    qpid::sys::sleep(1);

  sm.delBroker(broker);
  return 0;
}
