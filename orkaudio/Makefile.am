# not a GNU package. You can remove this line, if
# have all needed files, that a GNU package needs
AUTOMAKE_OPTIONS = foreign 1.4


sbin_PROGRAMS = orkaudio
orkaudio_LDADD =
orkaudio_LDFLAGS = -lsndfile -lACE -lxerces-c -llog4cxx -lorkbase 
orkaudio_SOURCES = OrkAudio.cpp AudioTape.cpp \
			BatchProcessing.cpp CapturePluginProxy.cpp \
			CapturePort.cpp Config.cpp ConfigManager.cpp \
			Daemon.cpp ImmediateProcessing.cpp LogManager.cpp \
			MultiThreadedServer.cpp Reporting.cpp
AM_CPPFLAGS = -D_REENTRANT
INCLUDES = -I@top_srcdir@ -I../orkbasecxx
SUBDIRS = audiocaptureplugins messages audiofile filters
orkaudio_LDADD = $(top_builddir)/messages/libmessages.la \
	$(top_builddir)/audiofile/libaudiofile.la 

orkaudioconfdir = /etc/orkaudio
orkaudioconf_DATA = \
	logging-linux-template.properties \
	config-linux-template.xml
install-orkaudioconfDATA:
	/bin/sh ./mkinstalldirs /etc/orkaudio
	cp logging-linux-template.properties /etc/orkaudio/logging.properties
	cp config-linux-template.xml /etc/orkaudio/config.xml
	/bin/sh ./mkinstalldirs /var/log/orkaudio


