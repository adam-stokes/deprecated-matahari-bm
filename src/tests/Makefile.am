
bin_PROGRAMS = \
	testhostapis

INCLUDES = \
	-I$(top_builddir)/lib \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/src

testhostapis_SOURCES = \
	$(top_srcdir)/src/host.cpp \
	$(top_srcdir)/src/host.h \
	$(top_srcdir)/src/linux_platform.cpp \
	$(top_srcdir)/src/linux_platform.h \
	$(top_srcdir)/src/platform.cpp \
	$(top_srcdir)/src/platform.h \
	testhostapis.cpp \
	testhostapis.h

testhostapis_LDFLAGS = -L/usr/local/lib
testhostapis_LDADD = -lcppunit $(LIBVIRT_LIBS) $(PCRE_LIBS) $(UDEV_LIBS)
testhostapis_LDADD += ../../lib/libmatahari.a
