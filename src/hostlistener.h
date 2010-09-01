#ifndef __HOSTLISTENER_H
#define __HOSTLISTENER_H

/* hostlistener.h - Copyright (C) 2009 Red Hat, Inc.
 * Written by Darryl Pierce <dpierce@redhat.com>.
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

/*
  HostListener defines a type which receives notification
  whenever a Host event occurs.
 */
class HostListener
{
 public:
  virtual void heartbeat(uint64_t timestamp, uint32_t sequence) = 0;
  virtual void updated() = 0;
};

#endif
