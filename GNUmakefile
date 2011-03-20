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
TARPREFIX	= $(PACKAGE)-$(PACKAGE)-$(TAG)
TARFILE		= $(TARPREFIX).tgz

RPM_ROOT	?= $(shell pwd)
RPM_OPTS	= --define "_sourcedir $(RPM_ROOT)" 	\
		  --define "_specdir   $(RPM_ROOT)" 	\
		  --define "_srcrpmdir $(RPM_ROOT)"

TAG    ?= $(shell git show --pretty="format:%h" --abbrev-commit | head -n 1)
WITH   ?= 
VARIANT ?=
PROFILE ?= fedora-14-x86_64

linux.build:
	@echo "=::=::=::= Setting up for Linux =::=::=::= "
	mkdir -p $@
	cd $@ && eval "`rpm --eval "%{cmake}" | grep -v -e "^%"`" -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
	@echo "Now enter the $@ directory and run 'make' as usual"

windows.build:
	@echo "=::=::=::= Setting up for Windows =::=::=::= "
	mkdir -p $@
	cd $@ && eval "`rpm --eval "%{_mingw32_cmake}"`" -DCMAKE_BUILD_TYPE=Release ..
	@echo "Now enter the $@ directory and run 'make' as usual"

%.check: %.build
	DESTDIR=`pwd`/$@ make -C $^ all install
	@echo "=::=::=::= Done =::=::=::= "
	@echo 
	@echo 
	@echo 

check:	
	rm -rf *.build *.check
	make linux.check windows.check

check-quick:   
	make linux.check windows.check

check-mock:   
	make mock mock-win

export:
	rm -f $(TARFILE).tgz
	git archive --prefix=$(TARPREFIX)/ $(TAG) | gzip > $(TARFILE)
	echo `date`: Rebuilt $(TARFILE) from $(TAG)

srpm:	export $(VARIANT)$(PACKAGE).spec
	rm -f *.src.rpm
	sed -i.sed 's/global\ specversion.*/global\ specversion\ $(shell expr 1 + $(lastword $(shell grep "global specversion" $(VARIANT)$(PACKAGE).spec)))/' $(VARIANT)$(PACKAGE).spec
	sed -i.sed 's/global\ upstream_version.*/global\ upstream_version\ $(firstword $(shell git show --pretty="format: %h"))/' $(VARIANT)$(PACKAGE).spec
	rpmbuild -bs $(RPM_OPTS) $(VARIANT)$(PACKAGE).spec

# eg. WITH="--with cman" make rpm
rpm:	srpm
	@echo To create custom builds, edit the flags and options in $(PACKAGE)-$(DISTRO).spec first
	rpmbuild $(RPM_OPTS) $(WITH) --rebuild $(RPM_ROOT)/*.src.rpm

overlay: export
	cp $(TARFILE) ~/rpmbuild/SOURCES
	cp $(VARIANT)$(PACKAGE).spec ~/rpmbuild/SPECS
	make -C ~/rpmbuild/SOURCES $(VARIANT)$(PACKAGE)

overlay-win:   
	make VARIANT=mingw32- overlay

mock-nodeps:
	-rm -rf $(RPM_ROOT)/mock
	mock --root=$(PROFILE) --resultdir=$(RPM_ROOT)/mock --rebuild $(RPM_ROOT)/*.src.rpm

mock:   srpm mock-nodeps

rpm-win:   
	make PROFILE=$(PROFILE) VARIANT=mingw32- rpm

mock-win:   
	make PROFILE=$(PROFILE) VARIANT=mingw32- srpm mock-nodeps

.PHONY: check linux.build windows.build
