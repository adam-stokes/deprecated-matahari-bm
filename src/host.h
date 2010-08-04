#ifndef __HOST_H
#define __HOST_H

/* host.h - Copyright (C) 2009 Red Hat, Inc.
 * Written by Arjun Roy <arroy@redhat.com>
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

#include <string>
#include <set>

#include "hostlistener.h"

using namespace std;

void host_register_listener(HostListener* listener);

void host_remove_istener(HostListener* listener);

void host_update_event();

string host_get_uuid();

string host_get_hostname();

string host_get_operating_system();

string host_get_hypervisor();

unsigned int host_get_platform();

string host_get_architecture();

unsigned int host_get_memory();

void host_get_load_averages(double& one, double& five, double& fifteen);

void host_identify(const unsigned int iterations);

void host_shutdown();

void host_reboot();

#endif // __HOST_H
