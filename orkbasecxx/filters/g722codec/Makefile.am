SUBDIRS =  

plugindir= $(prefix)/lib/orkaudio/plugins
plugin_LTLIBRARIES = libg722codec.la
libg722codec_la_SOURCES = G722Codec.cpp
AM_CFLAGS = -D__unix__ -Dlinux -DNDEBUG -DNO_SCRATCH_MEMORY_USED -DLINUX32  -Dlinux32
AM_CPPFLAGS = -D_REENTRANT -D__unix__ -Dlinux -DNDEBUG -DNO_SCRATCH_MEMORY_USED -DLINUX32  -Dlinux32
libg722codec_la_LDFLAGS = -module
libg722codec_la_LIBADD = -lACE -lxerces-c -llog4cxx -lorkbase \
		-lpthread -lm

INCLUDES = -I@top_srcdir@ -I../..
