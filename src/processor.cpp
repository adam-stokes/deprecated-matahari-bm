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

#include "config.h"
#include <fstream>
#include "host.h"
#include <limits.h>
#include <pcre.h>
#include "processor.h"
#include <stdexcept>

cpuinfo_t cpuinfo;
bool cpuinfo_t::initialized = false;

void
cpu_get_details()
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
		  else if (name == "flags")
		    {
		      string flags(value);

		      // if the cpuflags contain "lm" then it's a 64 bit CPU
		      // http://www.brandonhutchinson.com/Understanding_proc_cpuinfo.html
		      cpuinfo.wordsize = (flags.find(" lm ") ? 64 : 32);
		    }
		}
	    }
	}
      input.close();
      cpuinfo.cpus /= cpuinfo.cores;
    }
#elif defined WIN32
  LPFN_GLPI proc;
  DWORD ret_length;
  PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer, ptr;

  proc = (LPFN_GLPI) GetProcAddress(GetModuleHandle(TEXT("kernel32")),
				    "GetLogicalProcessorInformation");
  if(proc)
    {
      BOOL done = FALSE;

      while (!done)
	{
	  DWORD rc = proc(buffer, &ret_length);

	  if(rc == FALSE)
	    {
	      if(GetLastError() == ERROR_INSUFFICIENT_BUFFER)
		{
		  if(buffer)
		    {
		      free(buffer);
		      buffer = NULL;
		    }

		  buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)malloc(ret_length);
		}
	    }
	  else
	    {
	      done = TRUE;
	    }
	}

      ptr = buffer;

      DWORD offset = 0;

      while(offset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= ret_length)
	{
	  switch(ptr->Relationship)
	    {
	    case RelationProcessorCore:    cpuinfo.cores++; break;
	    case RelationProcessorPackage: cpuinfo.cpus++;  break;
	    }
	}

      if(buffer)
	{
	  free(buffer);
	  buffer = NULL;
	}
    }
#endif
}

unsigned int
cpu_get_wordsize()
{
  static unsigned int wordsize = 0;

  if(wordsize == 0)
    {
      cpu_get_details();
      return cpuinfo.wordsize;
    }

  return wordsize;
}
