#
# Copyright (C) 2008 Andrew Beekhof
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#

-include Makefile

PACKAGE		?= matahari
VERSION		?= 0.4.0
TARFILE		= $(PACKAGE)-$(VERSION).tbz2

RPM_ROOT	= $(shell pwd)
RPM_OPTS	= --define "_sourcedir $(RPM_ROOT)" 	\
		  --define "_specdir   $(RPM_ROOT)" 	\
		  --define "_srcrpmdir $(RPM_ROOT)" 	\

TAG    ?= next
WITH   ?= 
VARIANT ?=
PROFILE ?= fedora-13-x86_64

setup:
	@echo "Setting up for Linux..."
	-test ! -d build && mkdir build
	-test ! -e build/CMakeFiles && cd build && cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
	@echo "Now enter the build/ directory and run 'make' as usual"

setup-win:
	@echo "Setting up for Windows..."
	-test ! -d build-win && mkdir build-win
	-test ! -e build-win/CMakeFiles && cd build-win && mingw32-cmake ..
	@echo "Now enter the build-win/ directory and run 'make' as usual"

export:
	rm -f $(TARFILE)
	git archive --prefix=$(PACKAGE)-$(VERSION)/ $(TAG) | bzip2 > $(TARFILE)
	echo `date`: Rebuilt $(TARFILE)

srpm:	export $(VARIANT)$(PACKAGE).spec
	rm -f *.src.rpm
	sed -i.sed 's/global\ upstream_version.*/global\ upstream_version\ $(firstword $(shell git show --pretty="format: %h"))/' $(VARIANT)$(PACKAGE).spec
	rpmbuild -bs $(RPM_OPTS) $(VARIANT)$(PACKAGE).spec

# eg. WITH="--with cman" make rpm
rpm:	srpm
	@echo To create custom builds, edit the flags and options in $(PACKAGE)-$(DISTRO).spec first
	rpmbuild $(RPM_OPTS) $(WITH) --rebuild $(RPM_ROOT)/*.src.rpm

mock-nodeps:
	-rm -rf $(RPM_ROOT)/mock
	mock --root=$(PROFILE) --resultdir=$(RPM_ROOT)/mock --rebuild $(RPM_ROOT)/*.src.rpm

mock:   srpm mock-nodeps

mock-win:   
	make PROFILE=matahari VARIANT=mingw32- srpm mock-nodeps

.PHONY: build build-win
