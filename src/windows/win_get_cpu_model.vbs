' ---------------------------------------------------------------------
' win_get_cpu_model.vbs - Copyright (C) 2010, Red Hat.
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

strComputer   = "."
Set objWMIService = GetObject("winmgmts:\\" & strComputer & "\root\cimv2")
Set processors    = objWMIService.ExecQuery("select * from Win32_Processor")

displayed     = false

For Each processor in processors
    if Not displayed Then
	Wscript.echo "NAME=" & processor.Name
	displayed = true
    end if
Next
