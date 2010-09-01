/* host.cpp - Copyright (C) 2010 Red Hat, Inc.
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

#ifndef WIN32
#include "config.h"
#endif

#ifdef WIN32
#include <sstream>
#include <winsock.h>
#endif

#ifdef __linux__
#include <sys/sysinfo.h>
#include <sys/utsname.h>

// TODO remove this wrapper once rhbz#583747 is fixed
extern "C" {
#include <libudev.h>
}

#endif

#include <cstdlib>
#include <fstream>
#include "host.h"

#ifdef HAVE_LIBVIRT1
#include <libvirt/libvirt.h>
#endif

#include <limits.h>
#include "processor.h"
#include <set>
#include <string>

using namespace std;

set<HostListener*>   _listeners;
unsigned int         _heartbeat_sequence;

void
host_register_listener(HostListener* listener)
{
  _listeners.insert(listener);
}

void
host_remove_listener(HostListener* listener)
{
  _listeners.erase(listener);
}

void
host_update_event()
{
  _heartbeat_sequence++;

#ifdef WIN32
  // TODO: get the right date/time
  unsigned long __time;

  __time = 0L;
#elif defined __linux__
  time_t __time;

  time(&__time);
#endif

  for(set<HostListener*>::iterator iter = _listeners.begin();
      iter != _listeners.end();
      iter++)
    {
      (*iter)->heartbeat((unsigned long)__time,
			 _heartbeat_sequence);
    }

  for(set<HostListener*>::iterator iter = _listeners.begin();
      iter != _listeners.end();
      iter++)
    {
      (*iter)->updated();
    }
}

string
host_get_uuid()
{
  static string uuid;

  if(uuid.empty())
    {

#ifdef __linux__
      ifstream input("/var/lib/dbus/machine-id");

      if(input.is_open())
	{
	  getline(input, uuid);
	  input.close();
	}
#endif

    }

  return uuid;
}

string
host_get_hostname()
{
  static string hostname;

  if(hostname.empty())
    {
#ifdef __linux__

      struct utsname details;

      if(!uname(&details))
	{
	  hostname = string(details.nodename);
	}

#elif defined WIN32

      char buffer[512];
      WORD verreq;
      WSADATA wsadata;

      verreq = MAKEWORD(2, 2);
      if(!WSAStartup(verreq, &wsadata))
	{
	  if(!gethostname(buffer, 512))
	    {
	      hostname = string(buffer);
	    }
	  WSACleanup();
	}

 #endif
    }

  return hostname;
}

string
host_get_operating_system()
{
  static string operating_system;

  if(operating_system.empty())
    {
#ifdef __linux__

      struct utsname details;

      if(!uname(&details))
	{
	  operating_system = string(details.sysname) +
	    " (" + details.release + ")";
	}

#elif defined WIN32

      HINSTANCE dll;

      dll = LoadLibrary(TEXT("kernel32"));

      if(dll != NULL)
	{
	  typedef DWORD(WINAPI *version_function)(void);
	  version_function proc;
	  DWORD version;

	  proc = (version_function)GetProcAddress(dll,"GetVersion");

	  if(proc)
	    {
	      version = (*proc)();

	      DWORD major, minor, build;

	      major = (DWORD)(LOBYTE(LOWORD(version)));
	      minor = (DWORD)(HIBYTE(LOWORD(version)));
	      build = (DWORD)(HIWORD(version));

		  stringstream sstr;

		  sstr << "Windows (" << major << "." << minor << "." << build << ")";
		  operating_system = sstr.str();
	    }
	}

#endif
    }

  return operating_system;
}

string
host_get_hypervisor()
{
  static string hypervisor;

  if(hypervisor.empty())
    {

#ifdef HAVE_LIBVIRT1
      virConnectPtr lvconn = virConnectOpenReadOnly(NULL);

      if(lvconn)
	{
	  hypervisor = string(virConnectGetType(lvconn));
	  virConnectClose(lvconn);
	}
#endif

    }

  return hypervisor;
}

unsigned int
host_get_platform()
{
  static unsigned int wordsize = 0;

  if(wordsize == 0)
    {
      wordsize = sizeof(void *) * CHAR_BIT;
    }

  return wordsize;
}

string
host_get_architecture()
{
  static string architecture;

  if(architecture.empty())
    {
#ifdef __linux__

      struct utsname details;

      if(!uname(&details))
	{
	  architecture = string(details.machine);
	}

#elif defined WIN32

      _SYSTEM_INFO system_info;
      system_info.wProcessorArchitecture = PROCESSOR_ARCHITECTURE_UNKNOWN;
      GetSystemInfo(&system_info);

      switch(system_info.wProcessorArchitecture)
	{
	case PROCESSOR_ARCHITECTURE_AMD64:
	  architecture = "x86 (AMD)";
	  break;
	case PROCESSOR_ARCHITECTURE_IA64:
	  architecture = "ia64";
	  break;
	case PROCESSOR_ARCHITECTURE_INTEL:
	  architecture = "x86 (Intel)";
	  break;
	case PROCESSOR_ARCHITECTURE_UNKNOWN:
	  architecture = "Unknown";
	  break;
	}
#endif
    }

  return architecture;
}

unsigned int
host_get_memory()
{
  static unsigned int memory = 0;

  if(!memory)
    {
#ifdef __linux__

      struct sysinfo sysinf;
      if(!sysinfo(&sysinf))
	{
	  memory = sysinf.totalram / 1024L;
	}

#elif defined WIN32

      MEMORYSTATUS status;

      GlobalMemoryStatus(&status);
      memory = status.dwTotalPhys / 1024L;

#endif
    }

  return memory;
}

void
host_get_load_averages(double& one, double& five, double& fifteen)
{
#ifdef __linux__
  ifstream input;

  input.open("/proc/loadavg", ios::in);
  input >> one >> five >> fifteen;
  input.close();
#else
  one = 0.0;
  five = 0.0;
  fifteen = 0.0;
#endif
}

void
host_reboot()
{
}

void
host_shutdown()
{
}
