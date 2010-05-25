
bin_PROGRAMS = testmultiplexer

INCLUDES = \
	-I$(top_srcdir)/src

testmultiplexer_SOURCES = \
	$(top_srcdir)/src/virtio/multiplexer.cpp \
	$(top_srcdir)/src/virtio/multiplexer.h \
	testmultiplexer.cpp \
	testmultiplexer.h

testmultiplexer_LDADD = -lcppunit