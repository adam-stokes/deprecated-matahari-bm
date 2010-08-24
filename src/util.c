/* util.c - Copyright (C) 2010 Red Hat, Inc.
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

#include <stdio.h>
#include <stdlib.h>
#include "util.h"

#ifdef __linux__

#include <string.h>

#define POPEN_MODE "r"

#elif defined WIN32

#include <memory.h>

#define popen(fname, mode)  _popen(fname, mode)
#define pclose(x) _pclose(x)

#define POPEN_MODE "rt"

#endif

int
exec_and_capture_text(char* cmdline, char** output)
{
  int result = 1;

  // initial the output
  *output = NULL;

  if(cmdline != NULL)
    {
      FILE* input;
      int   offset = 0;

      if((input = popen(cmdline, POPEN_MODE)) != NULL)
	{
	  while(!feof(input))
	    {
	      char buffer[256];
	      int bytesread;

	      memset(buffer, 256, 0);
	      bytesread = fread(buffer, 1, 256, input);
	      *output = (char*)realloc(*output, offset + bytesread + 1);
	      memcpy((*output) + offset, buffer, bytesread);
	      offset += bytesread;
	      ((char*)(*output))[offset] = '\0';
	    }

	  pclose(input);
	  result = 0;
	}
    }

  return result;
}
