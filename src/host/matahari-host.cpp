/* matahari-host.cpp - Copyright (C) 2009 Red Hat, Inc.
 * Written by Arjun Roy <arroy@redhat.com>
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef WIN32
#include "config.h"
#endif

#include <set>
#include "matahari/mh_agent.h"
#include <qmf/Data.h>
#include "qmf/org/matahariproject/Host.h"
#include "qmf/org/matahariproject/EventHeartbeat.h"
#include "qmf/org/matahariproject/QmfPackage.h"

extern "C" {
#include <string.h>
#include <sigar.h>
#include "matahari/host.h"
#include "matahari/logging.h"
}

class HostAgent : public MatahariAgent
{
private:
    qmf::org::matahariproject::PackageDefinition _package;
public:
    int heartbeat();
    virtual int setup(qmf::AgentSession session);
    virtual gboolean invoke(qmf::AgentSession session, qmf::AgentEvent event,
		            gpointer user_data);
};

static gboolean heartbeat_timer(gpointer data)
{
    HostAgent *agent = (HostAgent *)data;
    g_timeout_add(agent->heartbeat(), heartbeat_timer, data);
    return FALSE;
}

int
main(int argc, char **argv)
{
    HostAgent *agent = new HostAgent();
    int rc = agent->init(argc, argv, "host");
    if (rc == 0) {
        heartbeat_timer(agent);
        agent->run();
    }

    return rc;
}

gboolean
HostAgent::invoke(qmf::AgentSession session, qmf::AgentEvent event, gpointer user_data)
{
    if (event.getType() != qmf::AGENT_METHOD) {
        session.methodSuccess(event);
        return TRUE;
    }

    const std::string& methodName(event.getMethodName());

    if (methodName == "shutdown") {
        host_shutdown();
    } else if (methodName == "reboot") {
        host_reboot();
    } else {
        session.raiseException(event, MH_NOT_IMPLEMENTED);
        goto bail;
    }

    session.methodSuccess(event);

bail:
    return TRUE;
}

int
HostAgent::setup(qmf::AgentSession session)
{
    _package.configure(session);
    _instance = qmf::Data(_package.data_Host);

    _instance.setProperty("update_interval", 5);
    _instance.setProperty("uuid", host_get_uuid());
    _instance.setProperty("hostname", host_get_hostname());
    _instance.setProperty("os", host_get_operating_system());
    _instance.setProperty("wordsize", host_get_cpu_wordsize());
    _instance.setProperty("arch", host_get_architecture());
    _instance.setProperty("memory", host_get_memory());
    _instance.setProperty("swap", host_get_swap());
    _instance.setProperty("cpu_count", host_get_cpu_count());
    _instance.setProperty("cpu_cores", host_get_cpu_number_of_cores());
    _instance.setProperty("cpu_model", host_get_cpu_model());
    _instance.setProperty("cpu_flags", host_get_cpu_flags());

    session.addData(_instance);
    return 0;
}

int
HostAgent::heartbeat()
{
    uint64_t timestamp = 0L, now = 0L;
    sigar_loadavg_t avg;
    sigar_proc_stat_t procs;
    static uint32_t _heartbeat_sequence = 0;
    uint32_t interval = _instance.getProperty("update_interval").asInt32();

    _heartbeat_sequence++;
    mh_trace("Updating stats: %d %d", _heartbeat_sequence, interval);

    if(interval == 0) {
        /* Updates disabled, check again in 5min */
        return 5*60*1000;
    }

#ifndef MSVC
    timestamp = ::time(NULL);
#endif

    now = timestamp * 1000000000;

    _instance.setProperty("last_updated", now);
    _instance.setProperty("sequence", _heartbeat_sequence);

    _instance.setProperty("free_swap", host_get_swap_free());
    _instance.setProperty("free_mem", host_get_mem_free());

    ::qpid::types::Variant::Map load;
    memset(&avg, 0, sizeof(sigar_loadavg_t));
    host_get_load_averages(&avg);
    load["1"]  = ::qpid::types::Variant((double)avg.loadavg[0]);
    load["5"]  = ::qpid::types::Variant((double)avg.loadavg[1]);
    load["15"] = ::qpid::types::Variant((double)avg.loadavg[2]);
    _instance.setProperty("load", load);

    ::qpid::types::Variant::Map proc;
    host_get_processes(&procs);
    proc["total"]    = ::qpid::types::Variant((int)procs.total);
    proc["idle"]     = ::qpid::types::Variant((int)procs.idle);
    proc["zombie"]   = ::qpid::types::Variant((int)procs.zombie);
    proc["running"]  = ::qpid::types::Variant((int)procs.running);
    proc["stopped"]  = ::qpid::types::Variant((int)procs.stopped);
    proc["sleeping"] = ::qpid::types::Variant((int)procs.sleeping);
    _instance.setProperty("process_statistics", proc);

    qmf::Data event = qmf::Data(_package.event_heartbeat);
    event.setProperty("timestamp", timestamp);
    event.setProperty("sequence", _heartbeat_sequence);
    _agent_session.raiseEvent(event);

    return interval * 1000;
}
