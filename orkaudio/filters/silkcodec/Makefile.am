METASOURCES = AUTO
plugindir= $(prefix)/lib/orkaudio/plugins
plugin_LTLIBRARIES = libsilkcodec.la
libsilkcodec_la_SOURCES = SilkCodec.cpp 
libsilkcodec_la_LDFLAGS = -module 
AM_CPPFLAGS = -D_REENTRANT
libsilkcodec_la_LIBADD =  -llog4cxx \
			-L/opt/silk/SILKCodec/SILK_SDK_SRC_FIX -lSKP_SILK_SDK		
				               
INCLUDES = -I@top_srcdir@ -I../../../orkbasecxx -I/opt/silk/SILKCodec/SILK_SDK_SRC_FIX/interface -I/opt/silk/SILKCodec/SILK_SDK_SRC_FIX/src  -I/usr/include/apr-1 -I/usr/include/apr-1.0

all-local: libsilkcodec.la
	cp .libs/libsilkcodec.so.0.0.0 ../../plugins/libsilkcodec.so
