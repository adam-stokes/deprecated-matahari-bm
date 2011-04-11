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

#include "matahari/mh_agent.h"
#include <qmf/Data.h>
#include <qmf/ConsoleEvent.h>
#include <qmf/ConsoleSession.h>
#include "qmf/org/matahariproject/Console.h"
#include <qpid/sys/Time.h>

#include <sstream>
using namespace std;

extern "C" {
#include <sigar.h>
#include "matahari/host.h"
}

class ConsoleAgent : public MatahariAgent
{
public:
    int setup(qmf::AgentSession session);
    gboolean invoke(qmf::AgentSession session, qmf::AgentEvent event, gpointer user_data);
};

int main(int argc, char** argv)
{
    ConsoleAgent *agent = new ConsoleAgent();;
    int rc = agent->init(argc, argv, "console");
    if(rc == 0) {
	agent->run();
    }
    return rc;
}

int
ConsoleAgent::setup(qmf::AgentSession session)
{
    _instance = qmf::Data(_package.data_Console);
    _instance.setProperty("uuid", host_get_uuid());
    _instance.setProperty("hostname", host_get_hostname());
    _instance.setProperty("status", "Waiting for console event");
    session.addData(_instance);

    qmf::ConsoleSession _console_session;
    _console_session = qmf::ConsoleSession(_amqp_connection);
    _console_session.open();
    qmf::Agent agent;
    while (true) {
	if(_console_session.getAgentCount() > 0) {
	    agent = _console_session.getAgent(0);
	    qmf::ConsoleEvent result(agent.query("{class:Console, package:'org.matahariproject', where:[eq, hostname, [quote, supa.kooba]]}"));
	    if(result.getType() == qmf::CONSOLE_QUERY_RESPONSE) {
		_instance.setProperty("status", "Received console event");
	    }
	}
	qpid::sys::sleep(1);
    }

    return 0;
}

gboolean
ConsoleAgent::invoke(qmf::AgentSession session, qmf::AgentEvent event, gpointer user_data)
{
    if(event.getType() == qmf::AGENT_METHOD) {
//	const std::string& methodName(event.getMethodName());
	goto bail;
    }
    session.methodSuccess(event);
bail:
    return TRUE;
}
