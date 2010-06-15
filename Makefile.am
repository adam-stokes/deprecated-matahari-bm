ACLOCAL_AMFLAGS = -I m4

SUBDIRS = lib src

rpm: clean
	@(unset CDPATH ; $(MAKE) dist && cp $(distdir).tar.gz \
	  $(HOME)/rpmbuild/SOURCES/ && rpmbuild -ba matahari.spec)

install-data-local: install-init
uninstall-local: uninstall-init

install-init: matahari.init
	mkdir -p $(DESTDIR)$(sysconfdir)/rc.d/init.d
	$(INSTALL_SCRIPT) matahari.init \
		$(DESTDIR)$(sysconfdir)/rc.d/init.d/matahari
	mkdir -p $(DESTDIR)$(sysconfdir)/sysconfig
	$(INSTALL_SCRIPT) matahari.sysconf \
	  $(DESTDIR)$(sysconfdir)/sysconfig/matahari

uninstall-init:
	rm -f $(DESTDIR)$(sysconfdir)/rc.d/init.d/matahari \
	$(DESTDIR)$(sysconfdir)/sysconfig/matahari

matahari.init: matahari.init.in
	sed						\
	    -e s!\@localstatedir\@!@localstatedir@!g	\
	    -e s!\@sbindir\@!@sbindir@!g		\
	    -e s!\@sysconfdir\@!@sysconfdir@!g		\
	    < $< > $@-t
	chmod a+x $@-t
	mv $@-t $@

EXTRA_DIST =	autogen.sh \
		matahari.init.in \
		matahari.sysconf

CLEANFILES = matahari.init
