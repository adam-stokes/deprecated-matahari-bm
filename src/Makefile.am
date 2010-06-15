NCLUDES = -I$(top_builddir)/lib -I$(top_srcdir)/lib -I$(top_srcdir)/src/qmf/com/redhat/matahari $(LIBVIRT_CFLAGS) $(PCRE_CFLAGS) $(UDEV_CFLAGS)

AM_CFLAGS = $(INCLUDES)
AM_CXXFLAGS = $(INCLUDES)

sbin_PROGRAMS = matahari

first = qmf/com/redhat/matahari/Host.cpp

generated_file_list = \
	qmf/com/redhat/matahari/EventHeartbeat.cpp \
	qmf/com/redhat/matahari/EventHeartbeat.h \
	qmf/com/redhat/matahari/Host.h \
	qmf/com/redhat/matahari/Package.cpp \
	qmf/com/redhat/matahari/Package.h

nodist_matahari_SOURCES = $(generated_file_list) $(first)

matahari_SOURCES = \
	host.cpp \
	host.h \
	hostlistener.h \
	linux_platform.cpp \
	linux_platform.h \
	main.cpp \
	platform.cpp \
	platform.h \
	qmf/hostagent.cpp \
	qmf/hostagent.h \
	virtio/multiplexer.cpp \
	virtio/multiplexer.h

$(generated_file_list): $(generated_file_list)

$(first): schema.xml $(generated_file_list)
	qmf-gen -o ./qmf ./schema.xml

BUILT_SOURCES = $(generated_file_list) $(first)
CLEANFILES = $(generated_file_list) $(first)

matahari_CPPFLAGS = -fno-strict-aliasing
matahari_LDADD = -lpcre -lqmf -lqpidclient -lqpidcommon -ludev -lvirt
matahari_LDADD += ../lib/libmatahari.a

dist_pkgdata_DATA = schema.xml
