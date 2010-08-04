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

#include <config.h>
#include <cstdlib>
#include <fstream>
#include "host.h"
#include <libvirt/libvirt.h>
#include <limits.h>
#include "processor.h"
#include <set>
#include <string>
#include <sys/sysinfo.h>

#ifdef __linux__

#include <sys/utsname.h>

#elif defined WIN32

#include <winsock.h>

#endif

// TODO remove this wrapper once rhbz#583747 is fixed
extern "C" {
#include <libudev.h>
}

using namespace std;

#ifdef WIN32

/* The following definitions make up for what is lacking currently
 * in the MinGW packages, and should be moved out to them at some
 * point in future.
 */

typedef enum _LOGICAL_PROCESSOR_RELATIONSHIP {
  RelationProcessorCore,
  RelationNumaNode,
  RelationCache,
  RelationProcessorPackage,
  RelationGroup,
  RelationAll                = 0xffff
} LOGICAL_PROCESSOR_RELATIONSHIP;

typedef enum _PROCESSOR_CACHE_TYPE {
  CacheUnified,
  CacheInstruction,
  CacheData,
  CacheTrace
} PROCESSOR_CACHE_TYPE;

typedef struct _CACHE_DESCRIPTOR {
  BYTE                 Level;
  BYTE                 Associativity;
  WORD                 LineSize;
  DWORD                Size;
  PROCESSOR_CACHE_TYPE Type;
} CACHE_DESCRIPTOR, *PCACHE_DESCRIPTOR;

typedef struct _SYSTEM_LOGICAL_PROCESSOR_INFORMATION {
  ULONG_PTR                      ProcessorMask;
  LOGICAL_PROCESSOR_RELATIONSHIP Relationship;
  union {
    struct {
      BYTE Flags;
    } ProcessorCore;
    struct {
      DWORD NodeNumber;
    } NumaNode;
    CACHE_DESCRIPTOR Cache;
    ULONGLONG        Reserved[2];
  } ;
} SYSTEM_LOGICAL_PROCESSOR_INFORMATION, *PSYSTEM_LOGICAL_PROCESSOR_INFORMATION;

typedef BOOL (WINAPI* LPFN_GLPI)(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION, PDWORD);

#endif

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

  time_t __time;
  time(&__time);

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

      dll = LoadLibrary("kernel32");

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

	      operating_system = string("Windows ") +
		"(" + major + "." + minor + "." + build + ")";
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

#ifdef HAVE_LIBVIRT
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

      LPSYSTEM_INFO system_info;

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
