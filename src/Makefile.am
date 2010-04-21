INCLUDES = -I$(top_srcdir)/src/qmf/com/redhat/matahari $(HAL_CFLAGS) $(LIBVIRT_CFLAGS) $(PCRE_CFLAGS) $(UDEV_CFLAGS)

sbin_PROGRAMS = matahari

first = qmf/com/redhat/matahari/Host.cpp

generated_file_list = \
	qmf/com/redhat/matahari/ArgsNICIdentify_nic.h \
	qmf/com/redhat/matahari/Host.h \
	qmf/com/redhat/matahari/NIC.cpp \
	qmf/com/redhat/matahari/NIC.h \
	qmf/com/redhat/matahari/Package.cpp \
	qmf/com/redhat/matahari/Package.h \
	qmf/com/redhat/matahari/Processors.cpp \
	qmf/com/redhat/matahari/Processors.h

nodist_matahari_SOURCES = $(generated_file_list) $(first)

matahari_SOURCES = \
	hal.cpp \
	hal.h \
	host.cpp \
	host.h \
	linux_platform.cpp \
	linux_platform.h \
	main.cpp \
	nic.cpp \
	nic.h \
	platform.cpp \
	platform.h \
	processors.cpp \
	processors.h

$(generated_file_list): $(generated_file_list)

$(first): schema.xml $(generated_file_list)
	qmf-gen -o ./qmf ./schema.xml

BUILT_SOURCES = $(generated_file_list) $(first)
CLEANFILES = $(generated_file_list) $(first)

matahari_CPPFLAGS = -fno-strict-aliasing
matahari_LDFLAGS = -L/usr/local/lib
matahari_LDADD = -lqmf $(HAL_LIBS) $(LIBVIRT_LIBS) $(PCRE_LIBS) $(UDEV_LIBS)

dist_pkgdata_DATA = schema.xml
