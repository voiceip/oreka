# not a GNU package. You can remove this line, if
# have all needed files, that a GNU package needs
AUTOMAKE_OPTIONS = foreign 1.4


lib_LTLIBRARIES = liborkbase.la
liborkbase_la_LDFLAGS = 
liborkbase_la_SOURCES = Filter.cpp g711.c \
			OrkBase.cpp Object.cpp ObjectFactory.cpp \
			OrkClient.cpp AudioCapture.cpp Utils.cpp 
#INCLUDES = -I/projects/ext/xmlrpc++/xmlrpc++0.7/src
SUBDIRS = messages serializers
liborkbase_la_LIBADD = $(top_builddir)/serializers/libserializers.la \
	$(top_builddir)/messages/libmessages.la
AM_CXXFLAGS = -D_REENTRANT
