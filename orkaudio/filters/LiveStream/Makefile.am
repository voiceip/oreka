METASOURCES = AUTO
plugindir= $(prefix)/lib/orkaudio/plugins
plugin_LTLIBRARIES = livestream.la
livestream_la_SOURCES = LiveStream.cpp srs_librtmp.cpp
livestream_la_LDFLAGS = -module
AM_CPPFLAGS = -D_REENTRANT 
livestream_la_LIBADD = -llog4cxx -L/usr/lib -lapr-1 -lpthread -ldl -lm -lcrypto -lstdc++
INCLUDES = -I@top_srcdir@ -I../../../orkbasecxx  -I/usr/include/apr-1 -I/usr/include/apr-1.0

all-local: livestream.la
	cp .libs/livestream.so.0.0.0 ../../plugins/livestream.so
