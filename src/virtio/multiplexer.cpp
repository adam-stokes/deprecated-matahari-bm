/* multiplexer.cpp - Copyright (C) 2010 Red Hat, Inc.
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

#include <config.h>
#include "multiplexer.h"

// the singleton
Multiplexer* Multiplexer::_instance = new Multiplexer();

typedef map<string, t_apifunction> APIMAP;

Multiplexer::Multiplexer()
{}

Multiplexer*
Multiplexer::instance()
{
  return _instance;
}

void
Multiplexer::registerAPI(string name, t_apifunction apifunction)
{
  _apis.insert(APIMAP::value_type(string(name), apifunction));
}

t_apifunction
Multiplexer::getAPI(string name)
{
  return (*_apis.find(name)).second;
}

string
Multiplexer::invokeAPI(string name, string input)
{
  string        result  = string("");
  t_apifunction funcptr = (*_apis.find(name)).second;

  if(funcptr)
    {
      result = funcptr(input.c_str());
    }

  return result;
}

