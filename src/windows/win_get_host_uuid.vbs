' ---------------------------------------------------------------------
' win_get_host_uuid.vbs - Copyright (C) 2010, Red Hat.
' Written by Darryl L. Pierce <dpierce@redhat.com>
'
' This program is free software: you can redistribute it and/or modify
' it under the terms of the GNU General Public License as published by
' the Free Software Foundation, either version 3 of the License, or
' (at your option) any later version.
'
' This program is distributed in the hope that it will be useful,
' but WITHOUT ANY WARRANTY; without even the implied warranty of
' MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
' GNU General Public License for more details.
'
' You should have received a copy of the GNU General Public License
' along with this program.  If not, see <http://www.gnu.org/licenses/>.
' ---------------------------------------------------------------------

strComputer = "."
displayed   = false

Set objWMIService = GetObject("winmgmts:\\" & strComputer & "\root\cimv2")
Set mboards       = objWMIService.ExecQuery("select * from Win32_BaseBoard")

For Each mboard in mboards
    if Not displayed Then
	Wscript.echo mboard.SerialNumber
	displayed = true
    end if
Next
