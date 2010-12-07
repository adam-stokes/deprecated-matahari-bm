/* host.cpp - Copyright (C) 2010 Red Hat, Inc.
 * Written by Darryl L. Pierce <dpierce@redhat.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <winsock.h>
#include <windows.h>
#include <winbase.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>

#include "matahari/logging.h"
#include "matahari/host.h"
#include "host_private.h"

#ifndef MSVC

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

#endif
typedef BOOL (WINAPI* LPFN_GLPI)(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION, PDWORD);

const char *
host_os_get_uuid(void)
{
    return host_os_get_hostname();
}

const char *
host_os_get_hostname(void)
{
  static char *hostname = NULL;

  if(hostname == NULL) {
      WORD verreq;
      WSADATA wsadata;
      hostname = malloc(512);

      verreq = MAKEWORD(2, 2);
      if(!WSAStartup(verreq, &wsadata)) {
	    gethostname(hostname, 511);
	    WSACleanup();
      }
  }

  return hostname;
}

const char *
host_os_get_operating_system(void)
{
  static char *operating_system = NULL;

  if(operating_system == NULL) {
      HINSTANCE dll = LoadLibrary(TEXT("kernel32"));

      if(dll != NULL) {
	  typedef DWORD(WINAPI *version_function)(void);
	  version_function proc;
	  DWORD version;
	  
	  proc = (version_function)GetProcAddress(dll,"GetVersion");
	  if(proc) {
	      version = (*proc)();
	      
	      DWORD major, minor, build;

	      major = (DWORD)(LOBYTE(LOWORD(version)));
	      minor = (DWORD)(HIBYTE(LOWORD(version)));
	      build = (DWORD)(HIWORD(version));

	      operating_system = malloc(512);
	      snprintf(operating_system, 511, "Windows (%lu.%lu.%lu)",
		       (unsigned long)major, (unsigned long)minor, (unsigned long)build);
	  }
      }
  }

  return operating_system;
}

const char *
host_os_get_architecture()
{
  static char *architecture = NULL;

  if(architecture == NULL) {
      SYSTEM_INFO system_info;
      system_info.wProcessorArchitecture = PROCESSOR_ARCHITECTURE_UNKNOWN;
      GetSystemInfo(&system_info);

      switch(system_info.wProcessorArchitecture) {
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
  }

  return architecture;
}

/* TODO: Replace with sigar calls */
unsigned int
host_os_get_memory()
{
  static unsigned int memory = 0;

  if(!memory) {
      MEMORYSTATUS status;

      GlobalMemoryStatus(&status);
      memory = status.dwTotalPhys / 1024L;
  }
  return memory;
}

/* TODO: Replace with sigar calls */
void
host_os_get_load_averages(double *one, double *five, double *fifteen)
{
    *one = 0.0;
    *five = 0.0;
    *fifteen = 0.0;
}

void
host_os_reboot()
{
}

void
host_os_shutdown()
{
}

void
host_os_get_cpu_details(void)
{
    LPFN_GLPI proc;
    DWORD ret_length;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer, ptr;

    if(cpuinfo.initialized) return;
    cpuinfo.initialized = 1;
  
    ptr    = NULL;
    buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)malloc(256);
    proc = (LPFN_GLPI) GetProcAddress(GetModuleHandle(TEXT("kernel32")),
				      "GetLogicalProcessorInformation");
    if(proc) {
	DWORD rc;
      retry:
	rc = proc(buffer, &ret_length);
	if(rc == FALSE) {
	    if(GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
		buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)realloc(buffer, ret_length + 1);
		goto retry;
	    }
	    
	    mh_err("Call to 'GetLogicalProcessorInformation' failed (%lu, %lu, %lu)",
		   rc, GetLastError(), ret_length);

	} else {
	    ptr = buffer;
	    DWORD offset = 0;
	    
	    while(offset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= ret_length) {
		switch(ptr->Relationship) {
		    case RelationProcessorCore:    cpuinfo.cores++; break;
		    case RelationProcessorPackage: cpuinfo.cpus++;  break;
		    default:
			break;
		}
		offset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
		ptr++;
	    }
	}
	
	// get the processor model
	cpuinfo.model = strdup("unknown");
    }

    free(buffer);
    buffer = NULL;
}
