METASOURCES = AUTO
noinst_LTLIBRARIES = libmessages.la
libmessages_la_SOURCES = AsyncMessage.cpp Message.cpp \
	SyncMessage.cpp CaptureMsg.cpp DeleteTapeMsg.cpp \
	PingMsg.cpp TapeMsg.cpp RecordMsg.cpp InitMsg.cpp \
	CrashMessage.cpp AddTagMsg.cpp

#libmessages_la_LIBADD = -L/projects/ext/xmlrpc++/xmlrpc++0.7/ -lXmlRpc
INCLUDES = -I@top_srcdir@ -I../../orkaudio
AM_CXXFLAGS = -D_REENTRANT
