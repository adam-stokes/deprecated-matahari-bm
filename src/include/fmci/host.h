/*
 * host.h
 *
 * Copyright (C) 2010 Red Hat, Inc.
 * Writen by Roman Rakus <rrakus@redhat.com>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef HOST_H_
#define HOST_H_

// Just for Host_identify, because host_identify is missing
#include <stdio.h>

gboolean
Host_identify(Fmci* fmci, GError** error)
{
  //host_identify(5);
  fprintf(stderr, "host_identify() is missing\n");
  return TRUE;
}

gboolean
Host_shutdown(Fmci* fmci, GError** error)
{
  host_shutdown();
  return TRUE;
}

gboolean
Host_reboot(Fmci* fmci, GError** error)
{
  host_reboot();
  return TRUE;
}

#endif /* HOST_H_ */
