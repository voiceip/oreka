METASOURCES = AUTO
noinst_LTLIBRARIES = libserializers.la
libserializers_la_SOURCES = DomSerializer.cpp DomSerializer.h Serializer.cpp\
	Serializer.h SingleLineSerializer.cpp SingleLineSerializer.h\
	UrlSerializer.cpp UrlSerializer.h XmlRpcSerializer.cpp\
	XmlRpcSerializer.h
#libserializers_la_LIBADD = -L/projects/ext/xmlrpc++/xmlrpc++0.7/ -lXmlRpc
INCLUDES = -I@top_srcdir@

AM_CXXFLAGS = -D_REENTRANT
