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
VERSION		?= 0.4.4
TARPREFIX	= $(PACKAGE)-$(PACKAGE)-$(TAG)
TARFILE		= $(TARPREFIX).tgz
HTML_ROOT	= coverity@www.clusterlabs.org:/var/www/html

RPM_ROOT	?= $(shell pwd)
RPM_OPTS	= --define "_sourcedir $(RPM_ROOT)" 	\
		  --define "_specdir   $(RPM_ROOT)" 	\
		  --define "_srcrpmdir $(RPM_ROOT)"

TAG    ?= $(shell git show --pretty="format:%h" --abbrev-commit | head -n 1)
WITH   ?= 
VARIANT ?=
PROFILE ?= fedora-16-x86_64

BUILD_COUNTER	?= build.counter
COUNT           = $(shell test ! -e $(BUILD_COUNTER) || echo $(shell expr 1 + $(shell cat $(BUILD_COUNTER))))

DOXYGEN:=$(shell which doxygen 2>/dev/null)
DOT:=$(shell which dot 2>/dev/null)

linux.build:
	@echo "=::=::=::= Setting up for Linux =::=::=::= "
	mkdir -p $@
	cd $@ && eval "`rpm --eval "%{cmake}" | grep -v -e "^%"`" -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
	@$(MAKE) --no-print-dir -C $@

tests: linux.build
	@if [ -f linux.build/src/tests/CTestTestfile.cmake ]; then \
		cd linux.build/src/tests && ctest -V; \
	fi

windows.build:
	@echo "=::=::=::= Setting up for Windows =::=::=::= "
	mkdir -p $@
	cd $@ && eval "`rpm --eval "%{_mingw32_cmake}"`" -DCMAKE_BUILD_TYPE=Release ..
	@$(MAKE) --no-print-dir -C $@

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

$(VARIANT)$(PACKAGE).spec: $(VARIANT)$(PACKAGE).spec.in
	cp $(VARIANT)$(PACKAGE).spec.in $(VARIANT)$(PACKAGE).spec

srpm:	export $(VARIANT)$(PACKAGE).spec
	rm -f *.src.rpm
	if [ -e $(BUILD_COUNTER) ]; then									\
		echo $(COUNT) > $(BUILD_COUNTER);								\
		sed -i.sed 's/global\ specversion.*/global\ specversion\ $(COUNT)/' $(VARIANT)$(PACKAGE).spec;  \
	else													\
		echo 1 > $(BUILD_COUNTER);									\
		sed -i.sed 's/global\ specversion.*/global\ specversion\ 1/' $(VARIANT)$(PACKAGE).spec; 	\
	fi
	sed -i.sed 's/global\ upstream_version.*/global\ upstream_version\ $(TAG)/' $(VARIANT)$(PACKAGE).spec
	rpmbuild -bs $(RPM_OPTS) $(VARIANT)$(PACKAGE).spec

# eg. WITH="--with cman" make rpm
rpm:	srpm
	@echo To create custom builds, edit the flags and options in $(PACKAGE)-$(DISTRO).spec first
	rpmbuild $(RPM_OPTS) $(WITH) --rebuild $(RPM_ROOT)/*.src.rpm

overlay: export
	sed -i.sed 's/global\ upstream_version.*/global\ upstream_version\ $(TAG)/' $(VARIANT)$(PACKAGE).spec
	cp $(TARFILE) ~/rpmbuild/SOURCES
	cp $(VARIANT)$(PACKAGE).spec ~/rpmbuild/SPECS
	make -C ~/rpmbuild/SOURCES $(VARIANT)$(PACKAGE)

overlay-win:   
	make VARIANT=mingw32- overlay

mock-nodeps:
	-rm -rf $(RPM_ROOT)/mock
	mock $(WITH) --root=$(PROFILE) --resultdir=$(RPM_ROOT)/mock --rebuild $(RPM_ROOT)/*.src.rpm

mock:   srpm mock-nodeps

rpm-win:   
	make PROFILE=$(PROFILE) VARIANT=mingw32- rpm

mock-win:   
	make PROFILE=$(PROFILE) VARIANT=mingw32- srpm mock-nodeps

COVERITY_DIR	 = $(shell pwd)/coverity-$(TAG)
COVHOST		?= coverity.example.com
COVPASS		?= password

coverity:
	rm -rf $(COVERITY_DIR) $(COVERITY_DIR).build
	mkdir -p $(COVERITY_DIR).build
	cd $(COVERITY_DIR).build && eval "`rpm --eval "%{cmake}" | grep -v -e "^%"`" -DWITH-DBUS=OFF -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
	cov-build --dir $(COVERITY_DIR) make -C $(COVERITY_DIR).build all
	@echo "Waiting for a Coverity license..."
	cov-analyze --dir $(COVERITY_DIR) --wait-for-license
	cov-format-errors --dir $(COVERITY_DIR) --emacs-style > $(TAG).coverity
	cov-format-errors --dir $(COVERITY_DIR)
	rsync -avzxlSD --progress $(COVERITY_DIR)/c/output/errors/ $(HTML_ROOT)/coverity/$(PACKAGE)/$(TAG)
#	rm -rf $(COVERITY_DIR) $(COVERITY_DIR).build

clean:
	rm -f *.tgz *.sed *.gres *~
	@if [ -d linux.build ] ; then \
		$(MAKE) --no-print-dir -C linux.build clean ; \
	elif [ -d windows.build ] ; then \
		$(MAKE) --no-print-dir -C windows.build clean ; \
	fi

tags:
	ctags --recurse -e src

doxygen-www: doxygen
	rsync -avzxlSD --progress doc/api/html/ $(HTML_ROOT)/doxygen/$(PACKAGE)/$(TAG)

doxygen:
ifeq ($(DOXYGEN),)
	@echo
	@echo "***********************************************"
	@echo "***                                         ***"
	@echo "*** You do not have doxygen installed.      ***"
	@echo "*** Please install it before generating the ***"
	@echo "*** developer documentation.                ***"
	@echo "***                                         ***"
	@echo "***********************************************"
	@exit 1
endif
ifeq ($(DOT),)
	@echo
	@echo "***********************************************"
	@echo "***                                         ***"
	@echo "*** You do not have graphviz installed.     ***"
	@echo "*** Please install it before generating the ***"
	@echo "*** developer documentation.                ***"
	@echo "***                                         ***"
	@echo "***********************************************"
	@exit 1
endif
	@cp doc/Doxyfile.in doc/Doxyfile
	@sed -i -e 's/###MATAHARI_VERSION###/$(VERSION)/' doc/Doxyfile
	@doxygen doc/Doxyfile

.PHONY: check linux.build windows.build clean doxygen tags www-doxygen coverity
