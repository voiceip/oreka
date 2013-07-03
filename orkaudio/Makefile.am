# not a GNU package. You can remove this line, if
# have all needed files, that a GNU package needs
AUTOMAKE_OPTIONS = foreign 1.4


sbin_PROGRAMS = orkaudio
orkaudio_LDADD =
orkaudio_LDFLAGS = -lACE -lxerces-c -llog4cxx -lorkbase -lsndfile -lorkspeex
orkaudio_SOURCES = 	OrkAudio.cpp
AM_CPPFLAGS = -D_REENTRANT
INCLUDES = -I@top_srcdir@ -I../orkbasecxx -I../orkbasecxx/filters/gsm -I../orkbasecxx/filters/gsm/gsm610 -I../orkbasecxx/filters/ilbc -I../orkbasecxx/filters/ilbc/ilbc -I../orkbasecxx/filters/audiogain -I../orkbasecxx/filters/g722codec -I../orkbasecxx/filters/speex -I../orkbasecxx/filters/g726codecs -I../orkbasecxx/filters/g726codecs/g72x
SUBDIRS = audiocaptureplugins filters
#orkaudio_LDADD = $(top_builddir)/messages/libmessages.la 

orkaudioconfdir = /etc/orkaudio
orkaudioconf_DATA = \
	logging-linux-template.properties \
	config-linux-template.xml
install-orkaudioconfDATA:
	mkdir -p /etc/orkaudio
	cp logging-linux-template.properties /etc/orkaudio/logging.properties
	cp config-linux-template.xml /etc/orkaudio/config.xml
	ldconfig
	mkdir -p /var/log/orkaudio


