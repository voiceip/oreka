METASOURCES = AUTO
noinst_LTLIBRARIES = libmessages.la
libmessages_la_SOURCES = CaptureMsg.cpp  DeleteTapeMsg.cpp  PingMsg.cpp  \
			TapeMsg.cpp TestMsg.cpp RecordMsg.cpp
AM_CPPFLAGS = -D_REENTRANT
libmessages_la_LIBADD = -L../../orkbasecxx/ -lorkbase
INCLUDES = -I@top_srcdir@ -I../../orkbasecxx
