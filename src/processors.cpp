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

#include "processors.h"
#include <fstream>
#include <iostream>
#include <pcre.h>

// TODO remove this wrapper once rhbz#583747 is fixed
extern "C" {
#include <libudev.h>
}

using namespace std;
namespace _qmf = qmf::com::redhat::matahari;

void
ProcessorsAgent::setup(ManagementAgent* agent, Manageable* parent)
{
  // setup the management object
  management_object = new _qmf::Processors(agent, this, parent);
  agent->addObject(management_object);

  int core_count = 0;
  string model = "unknown";

  struct udev* udev = udev_new();
  struct udev_enumerate* enumerator = udev_enumerate_new(udev);

  udev_enumerate_add_match_property(enumerator, "DRIVER", "processor");
  if(!udev_enumerate_scan_devices(enumerator))
    {
      struct udev_list_entry* entries = udev_enumerate_get_list_entry(enumerator);
      struct udev_list_entry* entry;

      udev_list_entry_foreach(entry, entries)
        {
          core_count++;
        }
    }

  udev_enumerate_unref(enumerator);
  udev_unref(udev);

  ifstream input("/proc/cpuinfo");
  if(input.is_open())
    {
      string regexstr = "(.*\\S)\\s*:\\s*(\\S.*)";
      int expected = 3;
      int found[expected * 3];
      const char* pcre_error;
      int pcre_error_offset;
      pcre* regex;
      bool done = false;
      bool started = false;

      regex = pcre_compile(regexstr.c_str(), 0, &pcre_error, &pcre_error_offset, NULL);
      if(!regex) { throw runtime_error("Unable to compile regular expression."); }

      while(!input.eof() && !done)
        {
          string line;

          getline(input, line);
          int match = pcre_exec(regex, NULL, line.c_str(), line.length(),
                                0, PCRE_NOTEMPTY,found, expected * 3);

          if(match == expected)
            {
              string name = line.substr(found[2], found[3] - found[2]);
              string value = line.substr(found[4], found[5] - found[4]);

              // if we're at a second processor and we've already started, then we're done
              if (name == "processor")
                {
                  if (started)
                    {
                      done = true;
                    }
                  else
                    {
                      started = true;
                    }
                }
              else
                {
                  if(name == "model name") model = value;
                }
            }
        }
      input.close();
    }

  // populate the managed object's values
  management_object->set_model(model);
  management_object->set_cores(core_count);
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
