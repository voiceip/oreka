METASOURCES = AUTO
lib_LTLIBRARIES = libgenerator.la
libgenerator_la_SOURCES = GeneratorConfig.cpp Generator.cpp \
	AudioCapturePluginCommon.cpp
libgenerator_la_LDFLAGS = -module
AM_CPPFLAGS = -D_REENTRANT
libgenerator_la_LIBADD = -lACE -lxerces-c -llog4cxx -lorkbase 
INCLUDES = -I@top_srcdir@ -I../../../orkbasecxx -I../common
AudioCapturePluginCommon.cpp:
	ln -s ../common/AudioCapturePluginCommon.cpp AudioCapturePluginCommon.cpp
