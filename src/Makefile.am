INCLUDES = -I$(top_srcdir)/src/qmf/com/redhat/matahari $(HAL_CFLAGS) $(BOOST_CPPFLAGS) $(LIBVIRT_CFLAGS)

sbin_PROGRAMS = matahari

generated_file_list = \
	qmf/com/redhat/matahari/Host.cpp\
	qmf/com/redhat/matahari/CPU.cpp\
	qmf/com/redhat/matahari/NIC.cpp\
	qmf/com/redhat/matahari/Package.cpp\
	qmf/com/redhat/matahari/Host.h\
	qmf/com/redhat/matahari/CPU.h\
	qmf/com/redhat/matahari/NIC.h\
	qmf/com/redhat/matahari/Package.h\
	qmf/com/redhat/matahari/ArgsNICIdentify_nic.h

nodist_matahari_SOURCES = $(generated_file_list)

matahari_SOURCES = \
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

matahari_LDFLAGS = -L/usr/local/lib
matahari_LDADD = -lqmfagent -lqmfcommon $(HAL_LIBS) $(BOOST_REGEX_LIB) $(LIBVIRT_LIBS)

dist_pkgdata_DATA = schema.xml
