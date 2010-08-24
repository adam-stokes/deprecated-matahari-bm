INCLUDES = \
	-I$(top_builddir)/lib \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/src/qmf/com/redhat/matahari \
	$(LIBVIRT_CFLAGS) $(PCRE_CFLAGS) $(UDEV_CFLAGS)

AM_CFLAGS = $(INCLUDES)
AM_CXXFLAGS = $(INCLUDES)

sbin_PROGRAMS = matahari

generated_file_list = \
	qmf/com/redhat/matahari/host/EventHeartbeat.cpp \
	qmf/com/redhat/matahari/host/EventHeartbeat.h \
	qmf/com/redhat/matahari/host/Host.cpp \
	qmf/com/redhat/matahari/host/Host.h \
	qmf/com/redhat/matahari/host/Package.cpp \
	qmf/com/redhat/matahari/host/Package.h \
	qmf/com/redhat/matahari/host/Processor.cpp \
	qmf/com/redhat/matahari/host/Processor.h

nodist_matahari_SOURCES = $(generated_file_list) config.h

matahari_SOURCES = \
	host.cpp \
	host.h \
	hostlistener.h \
	main.cpp \
	processor.cpp \
	processor.h \
	util.c \
	util.h \
	qmf/hostagent.cpp \
	qmf/hostagent.h \
	qmf/processoragent.cpp \
	qmf/processoragent.h \
	windows/win_get_cpu_model.vbs

SCHEMAS = \
	schema-host.xml

$(generated_file_list): $(SCHEMAS)
	qmf-gen -o ./qmf $(SCHEMAS)

BUILT_SOURCES = $(generated_file_list) $(windows_scripts)
CLEANFILES = $(generated_file_list)

matahari_CPPFLAGS = -fno-strict-aliasing
matahari_LDADD = -lpcre -lqmf -lqpidclient -lqpidcommon -ludev -lvirt
matahari_LDADD += ../lib/libmatahari.a

dist_pkgdata_DATA = $(SCHEMAS) $(generated_file_list) CMakeLists.txt
