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
#include <pcre.h>
#include <set>
#include <stdexcept>
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

typedef struct cpuinfo_
{
  static bool initialized;
  string model;
  unsigned int cpus;
  unsigned int cores;
} cpuinfo_t;

bool cpuinfo_t::initialized = false;

cpuinfo_t cpuinfo;

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
#endif

    }

  return memory;
}

void
host_get_cpu_details()
{
  if(cpuinfo.initialized) return;

  cpuinfo.initialized = true;

#if __linux__
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

      while(!input.eof())
	{
	  string line;

	  getline(input, line);
	  int match = pcre_exec(regex, NULL, line.c_str(), line.length(),
				0, PCRE_NOTEMPTY,found, expected * 3);

	  if(match == expected)
	    {
	      string name = line.substr(found[2], found[3] - found[2]);
	      string value = line.substr(found[4], found[5] - found[4]);

	      /* If we're at a second processor and we've already started,
		 then we're done.
	      */
	      if (name == "processor")
		{
		  cpuinfo.cpus++;
		  if (started)
		    {
		      done = true;
		    }
		  else
		    {
		      started = true;
		    }
		}
	      else if (!done)
		{
		  if      (name == "cpu cores")  cpuinfo.cores = atoi(value.c_str());
		  else if (name == "model name") cpuinfo.model = value;
		}
	    }
	}
      input.close();
      cpuinfo.cpus /= cpuinfo.cores;
    }
#endif

}

string
host_get_cpu_model()
{
  host_get_cpu_details();

  return cpuinfo.model;
}

unsigned int
host_get_number_of_cpus()
{
  host_get_cpu_details();

  return cpuinfo.cpus;
}

unsigned int
host_get_number_of_cpu_cores()
{
  host_get_cpu_details();

  return cpuinfo.cores;
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
