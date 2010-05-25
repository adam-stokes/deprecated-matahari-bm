#ifndef __MULTIPLEXER_H
#define __MULTIPLEXER_H

/* multiplexer.h - Copyright (C) 2010 Red Hat, Inc.
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

#include <map>
#include <string>

using namespace std;

typedef const char*(*t_apifunction)(const char*);

// Multiplexer processes the content received from a remote system.
class Multiplexer
{
 private:
  Multiplexer();

  static Multiplexer* _instance;

  map<string, t_apifunction> _apis;

 public:
  static Multiplexer* instance();

  void registerAPI(string apiname, t_apifunction apifunction);
  t_apifunction getAPI(string apiname);
  string invokeAPI(string apiname, string input);
};

#endif
