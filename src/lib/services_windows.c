/* 
 * Copyright (C) 2010 Andrew Beekhof <andrew@beekhof.net>
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
#include <windows.h> 
#include <tchar.h>
#include <stdio.h> 

#include "matahari/logging.h"
#include "matahari/mainloop.h"
#include "matahari/services.h"
#include "services_private.h"

/* Some code adapted from http://msdn.microsoft.com/en-us/library/ms682499(v=vs.85).aspx */
#define DIMOF(a)	((int) (sizeof(a)/sizeof(a[0])) )

static wchar_t *char2wide(const char *str)
{
    wchar_t *result = NULL;
    if(str != NULL) {
	size_t str_size = strlen(str) + 1;
	result = malloc(str_size * sizeof(wchar_t));
	mbstowcs(result, str, str_size);
    }
    return result;
}

static void create_service_key(const wchar_t *template, wchar_t *buffer, int size, char *name) 
{
#ifdef HAVE_SWPRINTF_S
	swprintf_s 
#else
	snwprintf
#endif
		( buffer, size, template, name);
}

static gboolean
windows_service_control(char *name, int command)
{
    HKEY     key_service;
    BOOL     fSuccess = TRUE;
    char     raw[100];
    size_t   value_bytes = 0;
    wchar_t *value = NULL;
    wchar_t  szRegistryPath[1024];

    create_service_key(
	L"SYSTEM\\CurrentControlSet\\services\\%s", szRegistryPath, DIMOF(szRegistryPath), name);

    fSuccess = RegOpenKey (HKEY_LOCAL_MACHINE, szRegistryPath, &key_service);
    if(fSuccess != ERROR_SUCCESS) {
	printf("Could not read key at '%ls' - %d\n\r", szRegistryPath, fSuccess);
	return FALSE;
    }

    itoa(command, raw, 10);
    value = char2wide(raw);
    value_bytes = 2 * (lstrlen(value) +1);

    fSuccess = RegSetValueEx (key_service, L"Start", 0, REG_SZ, (LPBYTE) value, value_bytes);
    mh_info("Setting Start='%ls' for %s: %s (%d)\n\r",
	    value, name, fSuccess==ERROR_SUCCESS?"PASS":"FAIL", fSuccess);
	
    RegCloseKey(key_service);
    free(value);
	
    return fSuccess==ERROR_SUCCESS;
}

#define BUFSIZE 4096
static char *read_output(HANDLE h) 
{
    /* Read output from the child process's pipe for STDOUT
     * Stop when there is no more data.
     */
    DWORD len = 0;
    DWORD bytes = 0; 
    char buf[BUFSIZE]; 
    char *data = NULL;
    
    while(1) { 
	if( ReadFile(h, buf, BUFSIZE, &bytes, NULL) == 0 ) {
	    break;
	} else if(bytes == 0) {
	    break;
	}
	
	data = realloc(data, len + bytes + 1);
	sprintf(data+len, "%s", buf);
	len += bytes;	
    }

    return data;
} 

gboolean
services_os_action_execute(svc_action_t* op, gboolean synchronous)
{
    STARTUPINFO siStartInfo;
    SECURITY_ATTRIBUTES saAttr; 
    PROCESS_INFORMATION piProcInfo; 

    HANDLE child_pipe_rd = NULL;
    HANDLE child_pipe_wr = NULL;


    if(strcmp("disable", op->action) == 0) {
	return windows_service_control(op->rsc, SERVICE_DISABLED);

    } else if(strcmp("enable", op->action) == 0) {
	return windows_service_control(op->rsc, SERVICE_AUTO_START);
    }
    
    /* For now, everything is synchronous */
    synchronous = TRUE;

    /* Initialize all structures */
    ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
    ZeroMemory( &saAttr, sizeof(SECURITY_ATTRIBUTES) );
    ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );
    
    /* Set the bInheritHandle flag so pipe handles are inherited.  */
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
    saAttr.bInheritHandle = TRUE; 
    saAttr.lpSecurityDescriptor = NULL; 

    /* Create a pipe for the child process's stdout  */
    if ( ! CreatePipe(&child_pipe_rd, &child_pipe_wr, &saAttr, 0) ) {
	mh_err("Couldn't create child pipe");
	return FALSE;
    }

    /* Ensure the read handle to the pipe for STDOUT is not inherited. */
    if ( ! SetHandleInformation(child_pipe_rd, HANDLE_FLAG_INHERIT, 0) ) {
	mh_err("Couldn't set pipe handle information"); 
	return FALSE;
    }
    
    siStartInfo.cb = sizeof(STARTUPINFO); 
    siStartInfo.hStdError = child_pipe_wr;
    siStartInfo.hStdOutput = child_pipe_wr;
/*   siStartInfo.hStdInput = g_hChildStd_IN_Rd; */
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
 
    if( !CreateProcess(NULL, 
		       op->opaque->exec,     // command line 
		       NULL,          // process security attributes 
		       NULL,          // primary thread security attributes 
		       TRUE,          // handles are inherited 
		       0,             // creation flags 
		       NULL,          // use parent's environment 
		       NULL,          // use parent's current directory 
		       &siStartInfo,  // STARTUPINFO pointer 
		       &piProcInfo)  // receives PROCESS_INFORMATION 
	) {
	mh_err("Could not create child process");
	return FALSE;
      
    } else if(synchronous) {
	DWORD status = 0;
	WaitForSingleObject(piProcInfo.hProcess, op->timeout /* Or INFINITE */);

	if(GetExitCodeProcess(piProcInfo.hProcess, &status) == 0) {
	    mh_err("Could not get exit code: %lu", GetLastError());
	    status = STILL_ACTIVE;
	}
       
	if(status == STILL_ACTIVE) {
	    TerminateProcess(piProcInfo.hProcess, 1);
	    mh_err("Operation for %s timeout out after %dms", op->id, op->timeout);
	}
       
	op->rc = status;
       
        /* Close the write end of the pipe before reading from the 
	 * read end of the pipe, to control child process execution.
	 * The pipe is assumed to have enough buffer space to hold the
	 * data the child process has already written to it.
	 */
	if (!CloseHandle(child_pipe_wr)) {
	    mh_err("Couldn't close write end of child pipe");
	}
       
	op->stdout_data = read_output( child_pipe_rd);
       
	CloseHandle(piProcInfo.hProcess);
	CloseHandle(piProcInfo.hThread);
    }

    return 0;
}

void
services_os_set_exec(svc_action_t* op)
{
    /* char *p = getenv("WINDIR"); */
    if(strcmp("status", op->action) == 0) {
	op->opaque->exec = g_strdup_printf("sc query %s", op->rsc);

    } else {
	/* op->opaque->exec = g_strdup_printf("%s\\system32\\sc %s %s", p, op->rsc, op->action); */
	op->opaque->exec = g_strdup_printf("sc %s %s", op->action, op->rsc);
    }
}

GList *
services_os_get_directory_list(const char *root, gboolean files)
{
    return NULL;
}

GList *services_os_list(void) 
{
    /* Need to implement */
    /* SC QUERY state= all | findstr "SERVICE_NAME"     */    
    return NULL;
}


GList *resources_os_list_ocf_providers(void) 
{
    return NULL;
}

GList *resources_os_list_ocf_agents(const char *provider) 
{
    return NULL;
}
