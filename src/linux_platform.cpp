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
#include <net/if.h>
#include <pcre.h>
#include <stdexcept>
#include <string.h>
#include <sys/ioctl.h>

// TODO remove this wrapper once rhbz#583747 is fixed
extern "C" {
#include <libudev.h>
}

#include "linux_platform.h"

LinuxPlatform::LinuxPlatform()
{
  int cpu_count = 0;
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
          cpu_count++;
        }
      setNumberOfCPUCores(cpu_count);
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
                  if(name == "model name") setCPUModel(value);
                }
            }
        }
      input.close();
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

vector<NetworkDeviceAgent>
LinuxPlatform::get_network_devices() const
{
  vector<NetworkDeviceAgent> result;

  DIR* entries = opendir("/sys/class/net");

  if(entries)
    {
      struct udev* udev = udev_new();
      struct dirent* entry;

      while(entry = (readdir(entries)))
        {
          string ifname = string(entry->d_name);
          if(ifname != "." && ifname != "..")
            {
              string fullpath = "/sys/class/net/" + ifname;
              struct udev_device* device = udev_device_new_from_syspath(udev,
                                                                        fullpath.c_str());

              if(udev_device_get_property_value(device, "ID_BUS"))
                {
                  int sock = socket(AF_INET, SOCK_DGRAM, 0);
                  struct ifreq ifr;
                  string vendor = string(udev_device_get_property_value(device, "ID_VENDOR_FROM_DATABASE"));
                  string model  = string(udev_device_get_property_value(device, "ID_MODEL_FROM_DATABASE"));

                  if(sock >= 0)
                    {
                      ifr.ifr_addr.sa_family = AF_INET;
                      strncpy(ifr.ifr_name, ifname.c_str(), IFNAMSIZ - 1);

                      if(!ioctl(sock, SIOCGIFHWADDR, &ifr))
                        {
                          char macaddr[256];

                          sprintf(macaddr,
                                  "%.2X:%.2X:%.2X:%.2X:%.2X:%.2X",
                                  (unsigned char )ifr.ifr_hwaddr.sa_data[0],
                                  (unsigned char )ifr.ifr_hwaddr.sa_data[1],
                                  (unsigned char )ifr.ifr_hwaddr.sa_data[2],
                                  (unsigned char )ifr.ifr_hwaddr.sa_data[3],
                                  (unsigned char )ifr.ifr_hwaddr.sa_data[4],
                                  (unsigned char )ifr.ifr_hwaddr.sa_data[5]);

                          result.push_back(NetworkDeviceAgent(ifname,
                                                              vendor,
                                                              model,
                                                              string(macaddr)));
                        }
                    }

                  udev_device_unref(device);
                }
            }
        }

      udev_unref(udev);
      closedir(entries);
   }

  return result;
}
