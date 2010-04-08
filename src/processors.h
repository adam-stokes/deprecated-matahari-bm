/* processor.h - Copyright (C) 2010 Red Hat, Inc.
 * Written by Darryl L. Pierce <dpierce@redhat.com>
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

#include <qpid/agent/ManagementAgent.h>
#include <qpid/management/Manageable.h>
#include <qpid/management/ManagementObject.h>
#include <qpid/agent/ManagementAgent.h>

#include "qmf/com/redhat/matahari/Processors.h"

using namespace qpid::management;
using namespace std;

using qpid::management::Manageable;

class ProcessorsAgent : public Manageable
{
 private:
  qmf::com::redhat::matahari::Processors* management_object;

 public:
  ProcessorsAgent() {}
  virtual ~ProcessorsAgent() {}

  ManagementObject* GetManagementObject(void) const { return management_object; }

  void setup(ManagementAgent* agent, Manageable* parent);

  // agent methods
  void update_load_averages(void) const;
};
