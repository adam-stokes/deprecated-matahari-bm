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

#ifdef WIN32
#  include <windows.h>
#  include <winbase.h>
#  ifndef MSVC

// Items missing from winnt.h from mingw32
//   http://msdn.microsoft.com/en-us/library/ms683194(VS.85).aspx

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

#  endif
typedef BOOL (WINAPI* LPFN_GLPI)(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION, PDWORD);

#else
#  include "config.h"
#endif

#include <fstream>
#include <iostream>
#include "host.h"
#include <limits.h>
#include <pcre.h>
#include "processor.h"
#include <stdexcept>
#include <string>
#include "util.h"

using namespace std;

typedef struct cpuinfo_
{
  static bool initialized;
  string model;
  unsigned int cpus;
  unsigned int cores;
  unsigned int wordsize;
} cpuinfo_t;

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

      regex = pcre_compile(regexstr.c_str(), 0, &pcre_error, &pcre_error_offset, NULL);
      if(!regex) { throw runtime_error("Unable to compile regular expression."); }

      while(!input.eof())
	{
	  string line;

	  getline(input, line);
	  int match = pcre_exec(regex, NULL, line.c_str(), line.length(),
				0, PCRE_NOTEMPTY,found, expected * 3);

	  if(match == expected) {
	      string name = line.substr(found[2], found[3] - found[2]);
	      string value = line.substr(found[4], found[5] - found[4]);

	      if (name == "processor") {
		  cpuinfo.cpus++;

	      } else if (name == "cpu cores")  {
		  cpuinfo.cores += atoi(value.c_str());

	      } else if (name == "model name") {
		  cpuinfo.model = value;
	      
	      } else if (name == "flags") {
		  string flags(value);
		  
		  // if the cpuflags contain "lm" then it's a 64 bit CPU
		  // http://www.brandonhutchinson.com/Understanding_proc_cpuinfo.html
		  cpuinfo.wordsize = (flags.find(" lm ") ? 64 : 32);
	      }
	  }
	}
      input.close();
    }
#elif defined WIN32
  LPFN_GLPI proc;
  DWORD ret_length;
  PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer, ptr;

  ptr    = NULL;
  buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)malloc(256);
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

		  buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)malloc(ret_length + 1);
		}
	      else
	      {
		  done = TRUE;
		  ret_length = 0;
		  cout << "Call to 'GetLogicalProcessorInformation' failed (" << rc << ", " << GetLastError() << ", " << ret_length << "): " << __FUNCTION__ << endl;
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
		default:
		    break;
	    }
	  offset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
	  ptr++;
	}

      if(buffer)
	{
	  free(buffer);
	  buffer = NULL;
	}

      // get the processor model
      cpuinfo.model = string("unknown");
      /*
      char* model = NULL;

      if(!exec_and_capture_text("cscript.exe /nologo win_get_cpu_model.vbs",
				model)
	{
	  cpuinfo.model = string(model);
	}

	 free(model);
      */
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

string
cpu_get_model()
{
  cpu_get_details();

  return cpuinfo.model;
}

unsigned int
cpu_get_count()
{
  cpu_get_details();

  return cpuinfo.cpus;
}

unsigned int
cpu_get_number_of_cores()
{
  cpu_get_details();

  return cpuinfo.cores;
}
