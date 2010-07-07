INCLUDES = -I$(top_srcdir)/src/qmf/com/redhat/matahari $(HAL_CFLAGS) $(LIBVIRT_CFLAGS) $(PCRE_CFLAGS)

sbin_PROGRAMS = matahari

first = qmf/com/redhat/matahari/Host.cpp

generated_file_list = \
	qmf/com/redhat/matahari/CPU.cpp\
	qmf/com/redhat/matahari/NIC.cpp\
	qmf/com/redhat/matahari/Package.cpp\
	qmf/com/redhat/matahari/Host.h\
	qmf/com/redhat/matahari/CPU.h\
	qmf/com/redhat/matahari/NIC.h\
	qmf/com/redhat/matahari/Package.h\
	qmf/com/redhat/matahari/ArgsNICIdentify_nic.h

nodist_matahari_SOURCES = $(generated_file_list) $(first)

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

$(generated_file_list): $(first)

$(first): schema.xml
	qmf-gen -o ./qmf ./schema.xml

BUILT_SOURCES = $(generated_file_list) $(first)
CLEANFILES = $(generated_file_list) $(first)

matahari_CPPFLAGS = -fno-strict-aliasing
matahari_LDFLAGS = -L/usr/local/lib
matahari_LDADD = \
	-lqmf \
	-lqpidcommon \
	-lqpidclient \
	$(HAL_LIBS) $(LIBVIRT_LIBS) $(PCRE_LIBS)

dist_pkgdata_DATA = schema.xml
