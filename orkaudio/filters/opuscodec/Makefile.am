METASOURCES = AUTO
plugindir= $(prefix)/lib/orkaudio/plugins
plugin_LTLIBRARIES = libopuscodec.la
libopuscodec_la_SOURCES = OpusCodec.cpp 
libopuscodec_la_LIBADD = -lopus
libopuscodec_la_LDFLAGS = -module
AM_CPPFLAGS = -D_REENTRANT
INCLUDES = -I@top_srcdir@ -I../../../orkbasecxx -I/usr/include/opus

all-local: libopuscodec.la
	cp .libs/libopuscodec.so.0.0.0 ../../plugins/libopuscodec.so
