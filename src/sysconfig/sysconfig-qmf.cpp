/* config-qmf.cpp - Copyright (C) 2011 Red Hat, Inc.
 * Written by Adam Stokes <astokes@fedoraproject.org>
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

/**
 * \file
 * \brief Sysconfig QMF Agent
 */

#ifndef WIN32
#include "config.h"
#endif

#include "matahari/mh_agent.h"

#include "qmf/org/matahariproject/QmfPackage.h"

#include <qpid/agent/ManagementAgent.h>

extern "C" {
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gprintf.h>
#include "matahari/sysconfig.h"
#include "matahari/logging.h"
#include "matahari/network.h"
#include "matahari/host.h"
#include <sigar.h>
#include <sigar_format.h>
};

static const char *configured_fn = "/var/lib/matahari/.is_configured";

class ConfigAgent : public MatahariAgent
{
private:
    qmf::org::matahariproject::PackageDefinition _package;
public:
    virtual int setup(qmf::AgentSession session);
    virtual gboolean invoke(qmf::AgentSession session, qmf::AgentEvent event,
                            gpointer user_data);
};

int
main(int argc, char **argv)
{
    ConfigAgent agent;
    int rc = agent.init(argc, argv, "Sysconfig");
    if (rc == 0) {
        agent.run();
    }
    return rc;
}

int
ConfigAgent::setup(qmf::AgentSession session)
{
    _package.configure(session);
    _instance = qmf::Data(_package.data_Sysconfig);

    _instance.setProperty("hostname", mh_hostname());
    _instance.setProperty("uuid", mh_uuid());
    _instance.setProperty("is_postboot_configured", 0);

    _agent_session.addData(_instance, "sysconfig");
    return 0;
}

static void
set_configured() {
    if(!g_file_test(configured_fn, G_FILE_TEST_EXISTS)) {
        g_creat(configured_fn, S_IRWXU);
    }
}

static int
is_configured(uint32_t flags) {

  if(flags & SYSCONFIG_FLAG_FORCE) {
    return 0;
  }

  if(g_file_test(configured_fn, G_FILE_TEST_EXISTS)) {
      return 1;
  }
  return 0;
}

gboolean
ConfigAgent::invoke(qmf::AgentSession session, qmf::AgentEvent event, gpointer user_data)
{
    int rc = 0;
    uint32_t flags;


    if (event.getType() != qmf::AGENT_METHOD) {
        return TRUE;
    }

    const std::string& methodName(event.getMethodName());

    if (methodName == "run_uri") {
      flags = event.getArguments()["flags"].asInt32();
        if((is_configured(flags)) == 0) {
            rc = mh_sysconfig_run_uri(event.getArguments()["uri"].asString().c_str(),
                flags,
                event.getArguments()["scheme"].asString().c_str());
            if (rc == 0) {
                set_configured();
            }
        }
        event.addReturnArgument("configured", rc);
    } else if (methodName == "run_string") {
      flags = event.getArguments()["flags"].asInt32();
      if((is_configured(flags)) == 0) {
        rc = mh_sysconfig_run_string(event.getArguments()["data"].asString().c_str(),
          flags,
          event.getArguments()["scheme"].asString().c_str());
        if (rc == 0) {
            set_configured();
        }
      }
      event.addReturnArgument("configured", rc);
    } else if (methodName == "query") {
      flags = event.getArguments()["flags"].asInt32();
      if((is_configured(flags)) == 0) {
        rc = mh_sysconfig_query(event.getArguments()["query"].asString().c_str(),
            event.getArguments()["data"].asString().c_str(),
            flags,
            event.getArguments()["scheme"].asString().c_str());
      }
      event.addReturnArgument("configured", rc);
    } else if (methodName == "is_configured") {
      rc = is_configured(0);
      _instance.setProperty("is_postboot_configured", rc);
      event.addReturnArgument("configured", rc);
    } else {
        session.raiseException(event, MH_NOT_IMPLEMENTED);
        goto bail;
    }

    session.methodSuccess(event);

bail:
    return TRUE;
}
