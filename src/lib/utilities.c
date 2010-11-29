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

#include <stdio.h>
#include <errno.h>

#include "matahari/logging.h"
#include "matahari/utilities.h"

#if __linux__
#include <sys/wait.h>
#include <sys/utsname.h>

int mh_log_level = LOG_INFO;

void mh_log_init(const char *ident, int level)
{
    mh_log_level = level;
    openlog(ident, LOG_NDELAY|LOG_PID, LOG_DAEMON);
}

void
mh_abort(const char *file, const char *function, int line,
	 const char *assert_condition, int do_core, int do_fork)
{
    int rc = 0;
    int pid = 0;
    int status = 0;

    if(do_core == 0) {
	mh_err("%s: Triggered assert at %s:%d : %s",
	       function, file, line, assert_condition);
	return;

    } else if(do_fork) {
	pid=fork();

    } else {
	mh_err("%s: Triggered fatal assert at %s:%d : %s",
	       function, file, line, assert_condition);
    }
	
    switch(pid) {
	case -1:
	    mh_crit("%s: Cannot create core for non-fatal assert at %s:%d : %s",
		    function, file, line, assert_condition);
	    return;

	default:	/* Parent */
	    mh_err("%s: Forked child %d to record non-fatal assert at %s:%d : %s",
		   function, pid, file, line, assert_condition);
	    do {
		rc = waitpid(pid, &status, 0);
		if(rc < 0 && errno != EINTR) {
		    mh_perror(LOG_ERR, "%s: Cannot wait on forked child %d", function, pid);
		}
			    
	    } while(rc < 0 && errno == EINTR);
	    
	    return;

	case 0:	/* Child */
	    abort();
	    break;
    }
}
#endif

#ifdef WIN32
int mh_log_level = 0;
#include <winsock.h>
#include <windows.h>
#include <winbase.h>
#endif

char *get_hostname(void)
{
  static char *hostname = NULL;

  if(hostname == NULL) {
#ifdef __linux__

      struct utsname details;

      if(!uname(&details)) {
	  hostname = strdup(details.nodename);
      }

#elif defined WIN32
      WORD verreq;
      WSADATA wsadata;
      
      hostname = malloc(500);
      verreq = MAKEWORD(2, 2);
      if(!WSAStartup(verreq, &wsadata)) {
	  if(gethostname(hostname, 500) != 0) {
	      free(hostname);
	      hostname = strdup("unknown");
	  }
	  WSACleanup();
      }
 #endif
  }

  return hostname;
}
