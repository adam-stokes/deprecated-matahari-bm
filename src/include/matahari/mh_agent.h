/* netagent.cpp - Copyright (C) 2010 Red Hat, Inc.
 * Written by Adam Stokes <astokes@fedoraproject.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.  A copy of the GNU General Public License is
 * also available at http://www.gnu.org/copyleft/gpl.html.
 */

#ifndef __MATAHARI_DAEMON_H
#define __MATAHARI_DAEMON_H

#include <string>
#include <qpid/sys/Time.h>
#include <qpid/agent/ManagementAgent.h>
#include <qpid/management/Manageable.h>

using namespace qpid::management;
using namespace std;

#include "qmf/com/redhat/matahari/Package.h"
namespace _qmf = qmf::com::redhat::matahari;

class MatahariAgent : public Manageable
{
  public:
    MatahariAgent() {};
    ~MatahariAgent() {};
    
    virtual int setup(ManagementAgent *agent) { return 0; };
    int init(int argc, char **argv);
};

#endif // __MATAHARI_DAEMON_H
