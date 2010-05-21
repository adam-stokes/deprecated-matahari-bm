#ifndef __PLATFORM_H
#define __PLATFORM_H

/* platform.h - Copyright (C) 2010 Red Hat, Inc.
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

#include <string>
#include <vector>
#include "networkdevice.h"

using namespace std;

/*
 * Platform defines a type that provides platform-specific details.
 *
 * Implementations provide the specific details needed by the
 * various agents at runtime.
 */
class Platform
{
 private:
  static Platform* _instance;

  string       _cpu_model;
  unsigned int _cpu_cores;

 protected:
  Platform() {}
  virtual~ Platform() {}

  void setCPUModel(const string model)      { _cpu_model = model; }
  void setNumberOfCPUCores(const int cores) { _cpu_cores = cores; }

 public:
  // the singleton instance
  static Platform* instance();

  string       getCPUModel() const         { return _cpu_model; }
  unsigned int getNumberOfCPUCores() const { return _cpu_cores; }

  virtual double getLoadAverage() const = 0;

  // returns the list of network devices for this platform
  virtual vector<NetworkDeviceAgent> get_network_devices() const = 0;
};

#endif
