INCLUDES = -I$(top_srcdir)/src/qmf/com/redhat/nodereporter $(HAL_CFLAGS) $(BOOST_CPPFLAGS) $(LIBVIRT_CFLAGS)

sbin_PROGRAMS = nodereporter

generated_file_list = \
	qmf/com/redhat/nodereporter/Host.cpp\
	qmf/com/redhat/nodereporter/CPU.cpp\
	qmf/com/redhat/nodereporter/NIC.cpp\
	qmf/com/redhat/nodereporter/Package.cpp\
	qmf/com/redhat/nodereporter/Host.h\
	qmf/com/redhat/nodereporter/CPU.h\
	qmf/com/redhat/nodereporter/NIC.h\
	qmf/com/redhat/nodereporter/Package.h\
	qmf/com/redhat/nodereporter/ArgsNICIdentify_nic.h

nodist_nodereporter_SOURCES = $(generated_file_list)

nodereporter_SOURCES = \
	cpu.cpp \
	cpu.h \
	hal.cpp \
	hal.h \
	host.cpp \
	host.h \
	main.cpp \
	nic.cpp \
	nic.h

$(generated_file_list): schema.xml
	export PYTHONPATH=/usr/local/lib/python2.6/site-packages/; \
	qmf-gen -o ./qmf ./schema.xml

BUILT_SOURCES = $(generated_file_list)
CLEANFILES = $(generated_file_list)

nodereporter_LDFLAGS = -L/usr/local/lib
nodereporter_LDADD = -lqmfagent -lqmfcommon $(HAL_LIBS) $(BOOST_REGEX_LIB) $(LIBVIRT_LIBS)

dist_pkgdata_DATA = schema.xml
