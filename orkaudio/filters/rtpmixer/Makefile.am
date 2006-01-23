METASOURCES = AUTO
lib_LTLIBRARIES = librtpmixer.la
librtpmixer_la_SOURCES = RtpMixer.cpp 
librtpmixer_la_LDFLAGS = -module
AM_CPPFLAGS = -D_REENTRANT
librtpmixer_la_LIBADD = -lACE -lxerces-c -llog4cxx -lorkbase
INCLUDES = -I@top_srcdir@ -I../../../orkbasecxx -I../common
