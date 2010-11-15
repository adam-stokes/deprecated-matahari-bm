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

#ifndef __NETAGENT_H
#define __NETAGENT_H

#include <qpid/management/Manageable.h>
#include <string>

#include "qmf/com/redhat/matahari/Network.h"

using namespace qpid::management;
using namespace std;

namespace _qmf = qmf::com::redhat::matahari;

class NetAgent : public Manageable
{
 private:
    ManagementAgent* _agent;
    _qmf::Network* _management_object;

 public:
    NetAgent(ManagementAgent* _agent);
    virtual ~NetAgent();
    
    static int setup(ManagementAgent* agent);
    ManagementObject* GetManagementObject() const { return _management_object; }
    status_t ManagementMethod(uint32_t method, Args& arguments, string& text);
};

#endif // __NETAGENT_H
