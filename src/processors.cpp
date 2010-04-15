/* processor.cpp - Copyright (C) 2010 Red Hat, Inc.
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

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>

#include <hal/libhal.h>

#include <pcre.h>

#include "hal.h"
#include "processors.h"

using namespace std;
namespace _qmf = qmf::com::redhat::matahari;

extern DBusError dbus_error;

void
ProcessorsAgent::setup(ManagementAgent* agent, Manageable* parent)
{
  // setup the management object
  management_object = new _qmf::Processors(agent, this, parent);
  agent->addObject(management_object);

  LibHalContext* context = get_hal_ctx();

  int num_results;
  char** processors = libhal_find_device_by_capability(context,"processor", &num_results, &dbus_error);

  if (!processors)
    throw runtime_error("Error: could not query processors via HAL.");

  // populate the managed object's values
  management_object->set_model(libhal_device_get_property_string(context, processors[0], "info.product", &dbus_error));
  management_object->set_cores(num_results);
}

void
ProcessorsAgent::update(void) const
{
  update_load_averages();
}

void
ProcessorsAgent::update_load_averages(void) const
{
  double load_average;
  ifstream input;

  input.open("/proc/loadavg", ios::in);
  input >> load_average;
  input.close();

  management_object->set_load_average(load_average);
}
