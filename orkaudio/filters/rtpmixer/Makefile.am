METASOURCES = AUTO
plugindir= $(prefix)/lib/orkaudio/plugins
plugin_LTLIBRARIES = librtpmixer.la
librtpmixer_la_SOURCES = RtpMixer.cpp 
librtpmixer_la_LDFLAGS = -module
AM_CPPFLAGS = -D_REENTRANT
librtpmixer_la_LIBADD = -lACE -lxerces-c -llog4cxx -lorkbase
INCLUDES = -I@top_srcdir@ -I../../../orkbasecxx -I../common
all-local: librtpmixer.la
	cp .libs/librtpmixer.so.0.0.0 ../../plugins/librtpmixer.so
