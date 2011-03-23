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
# include "config.h"
#endif

#include <qpid/console/ConsoleListener.h>
#include <qpid/console/SessionManager.h>
#include <qpid/sys/Time.h>

using namespace std;
using namespace qpid::console;

/* 
 * listener class for responding to the events raised by our
 * matahari agents
 */

class Listener : public ConsoleListner {
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

}

int main_int(int /*argc*/, char** /*argv*/)
{
  Listener listener;
  qpid::client::ConnectionSettings settings;

  cout << "Creating SessionManager" << endl;
  SessionManager sm;
  cout << "Adding broker" << endl;
  Broker* broker;

  broker = sm.addBroker(settings);
  broker->waitForStable();

  cout << "Package List:" << endl;
  vector<string> packages;
  sm.getPackages(packages);
  for (vector<string>::iterator iter = packages.begin(); iter != packages.end(); iter++) {
    cout << "    " << *iter << endl;
    SessionManager::KeyVector classKeys;
    sm.getClasses(classKeys, *iter);
    for (SessionManager::KeyVector::iterator cIter = classKeys.begin();
	 cIter != classKeys.end(); cIter++)
      cout << "        " << *cIter << endl;
  }

  Object::Vector list;
  cout << "getting exchanges..." << endl;
  sm.getObjects(list, "exchange");
  cout << "   returned " << list.size() << " elements" << endl;

  for (Object::Vector::iterator i = list.begin(); i != list.end(); i++) {
    cout << "exchange: " << *i << endl;
  }

  list.clear();
  cout << "getting queues..." << endl;
  sm.getObjects(list, "queue");
  cout << "   returned " << list.size() << " elements" << endl;

  for (Object::Vector::iterator i = list.begin(); i != list.end(); i++) {
    cout << "queue: " << *i << endl;
    cout << "  bindingCount=" << i->attrUint("bindingCount") << endl;
    cout << "  arguments=" << i->attrMap("arguments") << endl;
  }

  list.clear();
  sm.getObjects(list, "broker");
  if (list.size() == 1) {
    Object& broker = *list.begin();

    cout << "Broker: " << broker << endl;
    Object::AttributeMap args;
    MethodResponse result;
    args.addUint("sequence", 1);
    args.addString("body", "Testing...");

    cout << "Call echo method..." << endl;
    broker.invokeMethod("echo", args, result);
    cout << "Result: code=" << result.code << " text=" << result.text << endl;
    for (Object::AttributeMap::iterator aIter = result.arguments.begin();
	 aIter != result.arguments.end(); aIter++) {
      cout << "   Output Arg: " << aIter->first << " => " << aIter->second->str() << endl;
    }
  }

  sm.delBroker(broker);
  return 0;
}

int main(int argc, char** argv)
{
  try {
    return main_int(argc, argv);
  } catch(std::exception& e) {
    cout << "Top Level Exception: " << e.what() << endl;
  }
}
