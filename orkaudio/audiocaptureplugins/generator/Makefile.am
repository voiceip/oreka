METASOURCES = AUTO
lib_LTLIBRARIES = libgenerator.la
libgenerator_la_SOURCES = GeneratorConfig.cpp Generator.cpp \
	AudioCapturePluginCommon.cpp
libgenerator_la_LDFLAGS = -module
AM_CPPFLAGS = -D_REENTRANT
libgenerator_la_LIBADD = -lxerces-c -llog4cxx -lorkbase
INCLUDES = -I@top_srcdir@ -I../../../orkbasecxx -I../common -I/usr/include/apr-1 -I /usr/include/apr-1.0
AudioCapturePluginCommon.cpp:
	ln -s ../common/AudioCapturePluginCommon.cpp AudioCapturePluginCommon.cpp
