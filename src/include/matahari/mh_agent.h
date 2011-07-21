/* mh_agent.cpp - Copyright (C) 2010 Red Hat, Inc.
 * Written by Andrew Beekhof <andrew@beekhof.net>
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
 * \brief Matahari Agent API
 * \ingroup coreapi
 */

#ifndef __MATAHARI_DAEMON_H
#define __MATAHARI_DAEMON_H

#include <string>
#include <qpid/sys/Time.h>
#include <qpid/agent/ManagementAgent.h>
#include <qpid/management/Manageable.h>
#include <qpid/messaging/Connection.h>
#include <qmf/AgentEvent.h>
#include <qmf/AgentSession.h>
#include <qmf/Data.h>

extern "C" {
#include "matahari/mainloop.h"
#ifdef WIN32
int no_argument = 0;
int required_argument = 1;
#else
#  include <getopt.h>
#endif
}

#define MH_NOT_IMPLEMENTED "Not implemented"

#ifndef NS_MAXDNAME
# define NS_MAXDNAME 1025
#endif

using namespace qpid::management;
using namespace std;

namespace _qtype = ::qpid::types;

typedef struct mainloop_qmf_s {
        GSource source;
        qmf::AgentSession session;
        qmf::AgentEvent event;
        guint id;
        void *user_data;
        GDestroyNotify dnotify;
        gboolean (*dispatch)(qmf::AgentSession session, qmf::AgentEvent event,
                             gpointer user_data);
} mainloop_qmf_t;

extern int mh_add_option(
    int code, int has_arg, const char *name, const char *description,
    void *userdata, int(*callback)(int code, const char *name, const char *arg, void *userdata));

extern qpid::types::Variant::Map mh_parse_options(
    const char *proc_name, int argc, char **argv, qpid::types::Variant::Map &options);

extern mainloop_qmf_t *mainloop_add_qmf(int priority, qmf::AgentSession session,
        gboolean (*dispatch)(qmf::AgentSession session, qmf::AgentEvent event,
                             gpointer userdata),
        GDestroyNotify notify, gpointer userdata);

extern gboolean mainloop_destroy_qmf(mainloop_qmf_t *source);

class MatahariAgent
{
public:
    MatahariAgent() {};
    ~MatahariAgent() {};

    virtual int setup(qmf::AgentSession session) { return 0; };
    virtual gboolean invoke(qmf::AgentSession session, qmf::AgentEvent event,
                            gpointer user_data) { return FALSE; };
    int init(int argc, char **argv, const char* proc_name);
    void run();

protected:
    GMainLoop *mainloop;
    mainloop_qmf_t *qpid_source;

    qmf::Data _instance;
    qmf::AgentSession _agent_session;
    qpid::messaging::Connection _amqp_connection;
};

#endif // __MATAHARI_DAEMON_H
