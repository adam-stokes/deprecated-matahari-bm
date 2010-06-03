/* linux_platform.cpp - Copyright (C) 2010 Red Hat, Inc.
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

#include <fstream>
#include <iomanip>
#include <iostream>
#include <dirent.h>
#include <libvirt/libvirt.h>
#include <net/if.h>
#include <pcre.h>
#include <stdexcept>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>

using namespace std;

// TODO remove this wrapper once rhbz#583747 is fixed
extern "C" {
#include <libudev.h>
}

#include "linux_platform.h"

using namespace std;

LinuxPlatform::LinuxPlatform()
{
  struct utsname details;
  ifstream *input;

  input = new ifstream("/var/lib/dbus/machine-id");

  if(input->is_open())
    {
      string uuid;

      getline(*input, uuid);
      input->close();
      delete input;
      this->setUUID(uuid);
    }

  if(!uname(&details))
    {
      this->setHostname(string(details.nodename));
      this->setArchitecture(string(details.machine));
    }
  else
    {
      throw runtime_error("Unable to retrieve system details");
    }

  virConnectPtr lvconn = virConnectOpenReadOnly(NULL);

  if(lvconn)
    {
      this->setHypervisor(string(virConnectGetType(lvconn)));
      virConnectClose(lvconn);
    }

  struct sysinfo sysinf;
  if(!sysinfo(&sysinf))
    {
      this->setMemory(sysinf.totalram / 1024L);
    }
  else
    {
      throw runtime_error("Unable to retrieve system memory details.");
    }

  int cpu_count = 0;

  struct udev* udev = udev_new();
  struct udev_enumerate* enumerator = udev_enumerate_new(udev);

  udev_enumerate_add_match_property(enumerator, "DRIVER", "processor");
  if(!udev_enumerate_scan_devices(enumerator))
    {
      struct udev_list_entry* entries = udev_enumerate_get_list_entry(enumerator);
      struct udev_list_entry* entry;

      udev_list_entry_foreach(entry, entries)
        {
          cpu_count++;
        }
      setNumberOfCPUCores(cpu_count);
    }

  udev_enumerate_unref(enumerator);
  udev_unref(udev);

  input = new ifstream("/proc/cpuinfo");

  if(input->is_open())
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

      while(!input->eof() && !done)
        {
          string line;

          getline(*input, line);
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
                  if(name == "model name") setCPUModel(value);
                }
            }
        }
      input->close();
      delete input;
    }
}

double
LinuxPlatform::getLoadAverage() const
{
  double load_average;
  ifstream input;

  input.open("/proc/loadavg", ios::in);
  input >> load_average;
  input.close();

  return load_average;
}
