METASOURCES = AUTO
plugindir= $(prefix)/lib/orkaudio/plugins
plugin_LTLIBRARIES = librtpmixer.la
librtpmixer_la_SOURCES = RtpMixer.cpp 
librtpmixer_la_LDFLAGS = -module
AM_CPPFLAGS = -D_REENTRANT
librtpmixer_la_LIBADD = -lxerces-c -llog4cxx -lorkbase
INCLUDES = -I@top_srcdir@ -I../../../orkbasecxx -I../common  -I/usr/include/apr-1 -I/usr/include/apr-1.0
all-local: librtpmixer.la
	cp .libs/librtpmixer.so.0.0.0 ../../plugins/librtpmixer.so
