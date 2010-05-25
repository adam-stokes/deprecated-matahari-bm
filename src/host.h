#ifndef __HOST_H
#define __HOST_H

/* host.h - Copyright (C) 2009 Red Hat, Inc.
 * Written by Arjun Roy <arroy@redhat.com>
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
#include <set>

#include "hostlistener.h"
#include "processors.h"
#include "networkdevice.h"

using namespace std;

/*
  Host represents the public contract for the set of host APIs.
 */
class Host
{
 private:
  string          _uuid;
  string          _hostname;
  string          _hypervisor;
  string          _architecture;
  unsigned int    _memory;
  bool            _beeping;
  unsigned int    _heartbeat_sequence;

  Processors                 _processors;
  vector<NetworkDeviceAgent> _networkdevices;
  set<HostListener*>         _listeners;

 public:
  Host();
  virtual ~Host() {}

  void update();

  void addHostListener(HostListener*);
  void removeHostListener(HostListener*);

  Processors& getProcessors();

  string getUUID() const;
  string getHostname() const;
  string getHypervisor() const;
  string getArchitecture() const;
  unsigned int getMemory() const;

  bool isBeeping() const;
  void identify(const int iterations);
  void shutdown();
  void reboot();
};

#endif // __HOST_H
