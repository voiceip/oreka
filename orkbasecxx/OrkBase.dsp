# Microsoft Developer Studio Project File - Name="OrkBase" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=OrkBase - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "OrkBase.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "OrkBase.mak" CFG="OrkBase - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "OrkBase - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "OrkBase - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "OrkBase - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /Zi /O2 /Ob0 /I "." /I ".\filters\gsm" /I ".\filters\gsm\gsm610" /I ".\filters\ilbc" /I ".\filters\ilbc\ilbc" /I "C:\devExt\libsndfile\src" /I "C:\devExt\boost\boost_1_32_0" /I "C:\devExt\ACE_wrappers" /I "C:\devExt\xerces++\xerces-c_2_6_0-windows_nt-msvc_60\include" /I "C:\devExt\log4cxx\log4cxx-0.9.7\include" /I ".\filters\g722codec" /D "BUILD_ORKBASE" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D inline=__inline /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 LIBSNDFILE.lib xerces-c_2.lib log4cxx.lib ace.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /map /debug /machine:I386 /libpath:"C:\devExt\libsndfile\Release" /libpath:"C:\devExt\ACE_wrappers\lib" /libpath:"C:\devExt\log4cxx\log4cxx-0.9.7\msvc\Lib\Release" /libpath:"C:\devExt\xerces++\xerces-c_2_6_0-windows_nt-msvc_60\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=copy Release\OrkBase.dll ..\OrkAudio\OrkBase.dll
# End Special Build Tool

!ELSEIF  "$(CFG)" == "OrkBase - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "." /I ".\filters\gsm" /I ".\filters\gsm\gsm610" /I ".\filters\ilbc" /I ".\filters\ilbc\ilbc" /I "C:\devExt\libsndfile\src" /I "C:\devExt\boost\boost_1_32_0" /I "C:\devExt\ACE_wrappers" /I "C:\devExt\xerces++\xerces-c_2_6_0-windows_nt-msvc_60\include" /I "C:\devExt\log4cxx\log4cxx-0.9.7\include" /I ".\filters\g722codec" /D "BUILD_ORKBASE" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D inline=__inline /FR /FD /GZ /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 LIBSNDFILE.lib xerces-c_2D.lib log4cxx.lib aced.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /map /debug /machine:I386 /out:"Debug/OrkBaseD.dll" /pdbtype:sept /libpath:"C:\devExt\libsndfile\Debug" /libpath:"C:\devExt\ACE_wrappers\lib" /libpath:"C:\devExt\log4cxx\log4cxx-0.9.7\msvc\Lib\Debug" /libpath:"C:\devExt\xerces++\xerces-c_2_6_0-windows_nt-msvc_60\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=copy Debug\OrkBaseD.dll ..\OrkAudio\OrkBaseD.dll
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "OrkBase - Win32 Release"
# Name "OrkBase - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "Serializers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Serializers\DomSerializer.cpp

!IF  "$(CFG)" == "OrkBase - Win32 Release"

!ELSEIF  "$(CFG)" == "OrkBase - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Serializers\DomSerializer.h
# End Source File
# Begin Source File

SOURCE=.\Serializers\Serializer.cpp

!IF  "$(CFG)" == "OrkBase - Win32 Release"

!ELSEIF  "$(CFG)" == "OrkBase - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Serializers\Serializer.h
# End Source File
# Begin Source File

SOURCE=.\Serializers\SingleLineSerializer.cpp

!IF  "$(CFG)" == "OrkBase - Win32 Release"

!ELSEIF  "$(CFG)" == "OrkBase - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Serializers\SingleLineSerializer.h
# End Source File
# Begin Source File

SOURCE=.\Serializers\UrlSerializer.cpp
# End Source File
# Begin Source File

SOURCE=.\Serializers\UrlSerializer.h
# End Source File
# Begin Source File

SOURCE=.\Serializers\XmlRpcSerializer.cpp

!IF  "$(CFG)" == "OrkBase - Win32 Release"

!ELSEIF  "$(CFG)" == "OrkBase - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Serializers\XmlRpcSerializer.h
# End Source File
# End Group
# Begin Group "Messages"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Messages\AsyncMessage.cpp

!IF  "$(CFG)" == "OrkBase - Win32 Release"

!ELSEIF  "$(CFG)" == "OrkBase - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Messages\AsyncMessage.h
# End Source File
# Begin Source File

SOURCE=.\messages\CaptureMsg.cpp
# End Source File
# Begin Source File

SOURCE=.\messages\CaptureMsg.h
# End Source File
# Begin Source File

SOURCE=.\messages\DeleteTapeMsg.cpp
# End Source File
# Begin Source File

SOURCE=.\messages\DeleteTapeMsg.h
# End Source File
# Begin Source File

SOURCE=.\messages\InitMsg.cpp
# End Source File
# Begin Source File

SOURCE=.\messages\InitMsg.h
# End Source File
# Begin Source File

SOURCE=.\Messages\Message.cpp

!IF  "$(CFG)" == "OrkBase - Win32 Release"

!ELSEIF  "$(CFG)" == "OrkBase - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Messages\Message.h
# End Source File
# Begin Source File

SOURCE=.\messages\PingMsg.cpp
# End Source File
# Begin Source File

SOURCE=.\messages\PingMsg.h
# End Source File
# Begin Source File

SOURCE=.\messages\RecordMsg.cpp
# End Source File
# Begin Source File

SOURCE=.\messages\RecordMsg.h
# End Source File
# Begin Source File

SOURCE=.\Messages\SyncMessage.cpp

!IF  "$(CFG)" == "OrkBase - Win32 Release"

!ELSEIF  "$(CFG)" == "OrkBase - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Messages\SyncMessage.h
# End Source File
# Begin Source File

SOURCE=.\messages\TapeMsg.cpp
# End Source File
# Begin Source File

SOURCE=.\messages\TapeMsg.h
# End Source File
# Begin Source File

SOURCE=.\messages\TestMsg.cpp
# End Source File
# Begin Source File

SOURCE=.\messages\TestMsg.h
# End Source File
# End Group
# Begin Group "audiofile"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\audiofile\AudioFile.cpp
# End Source File
# Begin Source File

SOURCE=.\audiofile\AudioFile.h
# End Source File
# Begin Source File

SOURCE=.\audiofile\LibSndFileFile.cpp
# End Source File
# Begin Source File

SOURCE=.\audiofile\LibSndFileFile.h
# End Source File
# Begin Source File

SOURCE=.\audiofile\MediaChunkFile.cpp
# End Source File
# Begin Source File

SOURCE=.\audiofile\MediaChunkFile.h
# End Source File
# Begin Source File

SOURCE=.\audiofile\PcmFile.cpp
# End Source File
# Begin Source File

SOURCE=.\audiofile\PcmFile.h
# End Source File
# End Group
# Begin Group "filters"

# PROP Default_Filter ""
# Begin Group "gsm"

# PROP Default_Filter ""
# Begin Group "gsm610"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\filters\gsm\gsm610\add.c
# End Source File
# Begin Source File

SOURCE=.\filters\gsm\gsm610\code.c
# End Source File
# Begin Source File

SOURCE=.\filters\gsm\gsm610\config.h
# End Source File
# Begin Source File

SOURCE=.\filters\gsm\gsm610\decode.c
# End Source File
# Begin Source File

SOURCE=.\filters\gsm\gsm610\gsm.h
# End Source File
# Begin Source File

SOURCE=.\filters\gsm\gsm610\gsm610_priv.h
# End Source File
# Begin Source File

SOURCE=.\filters\gsm\gsm610\gsm_create.c
# End Source File
# Begin Source File

SOURCE=.\filters\gsm\gsm610\gsm_decode.c
# End Source File
# Begin Source File

SOURCE=.\filters\gsm\gsm610\gsm_destroy.c
# End Source File
# Begin Source File

SOURCE=.\filters\gsm\gsm610\gsm_encode.c
# End Source File
# Begin Source File

SOURCE=.\filters\gsm\gsm610\gsm_option.c
# End Source File
# Begin Source File

SOURCE=.\filters\gsm\gsm610\long_term.c
# End Source File
# Begin Source File

SOURCE=.\filters\gsm\gsm610\lpc.c
# End Source File
# Begin Source File

SOURCE=.\filters\gsm\gsm610\preprocess.c
# End Source File
# Begin Source File

SOURCE=.\filters\gsm\gsm610\rpe.c
# End Source File
# Begin Source File

SOURCE=.\filters\gsm\gsm610\short_term.c
# End Source File
# Begin Source File

SOURCE=.\filters\gsm\gsm610\table.c
# End Source File
# End Group
# Begin Source File

SOURCE=.\filters\gsm\GsmFilters.cpp
# End Source File
# Begin Source File

SOURCE=.\filters\gsm\GsmFilters.h
# End Source File
# End Group
# Begin Group "ilbc"

# PROP Default_Filter ""
# Begin Group "ilbc No. 1"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\filters\ilbc\ilbc\anaFilter.c
# End Source File
# Begin Source File

SOURCE=.\filters\ilbc\ilbc\constants.c
# End Source File
# Begin Source File

SOURCE=.\filters\ilbc\ilbc\createCB.c
# End Source File
# Begin Source File

SOURCE=.\filters\ilbc\ilbc\doCPLC.c
# End Source File
# Begin Source File

SOURCE=.\filters\ilbc\ilbc\enhancer.c
# End Source File
# Begin Source File

SOURCE=.\filters\ilbc\ilbc\FrameClassify.c
# End Source File
# Begin Source File

SOURCE=.\filters\ilbc\ilbc\gainquant.c
# End Source File
# Begin Source File

SOURCE=.\filters\ilbc\ilbc\getCBvec.c
# End Source File
# Begin Source File

SOURCE=.\filters\ilbc\ilbc\helpfun.c
# End Source File
# Begin Source File

SOURCE=.\filters\ilbc\ilbc\hpInput.c
# End Source File
# Begin Source File

SOURCE=.\filters\ilbc\ilbc\hpOutput.c
# End Source File
# Begin Source File

SOURCE=.\filters\ilbc\ilbc\iCBConstruct.c
# End Source File
# Begin Source File

SOURCE=.\filters\ilbc\ilbc\iCBSearch.c
# End Source File
# Begin Source File

SOURCE=.\filters\ilbc\ilbc\iLBC_decode.c
# End Source File
# Begin Source File

SOURCE=.\filters\ilbc\ilbc\iLBC_encode.c
# End Source File
# Begin Source File

SOURCE=.\filters\ilbc\ilbc\iLBC_filter.c
# End Source File
# Begin Source File

SOURCE=.\filters\ilbc\ilbc\LPCdecode.c
# End Source File
# Begin Source File

SOURCE=.\filters\ilbc\ilbc\LPCencode.c
# End Source File
# Begin Source File

SOURCE=.\filters\ilbc\ilbc\lsf.c
# End Source File
# Begin Source File

SOURCE=.\filters\ilbc\ilbc\packing.c
# End Source File
# Begin Source File

SOURCE=.\filters\ilbc\ilbc\StateConstructW.c
# End Source File
# Begin Source File

SOURCE=.\filters\ilbc\ilbc\StateSearchW.c
# End Source File
# Begin Source File

SOURCE=.\filters\ilbc\ilbc\syntFilter.c
# End Source File
# End Group
# Begin Source File

SOURCE=.\filters\ilbc\IlbcFilters.cpp
# End Source File
# Begin Source File

SOURCE=.\filters\ilbc\IlbcFilters.h
# End Source File
# End Group
# Begin Group "audiogain"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\filters\audiogain\AudioGain.cpp
# End Source File
# Begin Source File

SOURCE=.\filters\audiogain\AudioGain.h
# End Source File
# End Group
# Begin Group "g722codec"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\filters\g722codec\G722.h
# End Source File
# Begin Source File

SOURCE=.\filters\g722codec\G722Codec.cpp
# End Source File
# Begin Source File

SOURCE=.\filters\g722codec\G722Codec.h
# End Source File
# End Group
# End Group
# Begin Source File

SOURCE=.\AudioCapture.cpp
# End Source File
# Begin Source File

SOURCE=.\AudioCapture.h
# End Source File
# Begin Source File

SOURCE=.\AudioCapturePlugin.h
# End Source File
# Begin Source File

SOURCE=.\AudioTape.cpp
# End Source File
# Begin Source File

SOURCE=.\AudioTape.h
# End Source File
# Begin Source File

SOURCE=.\BatchProcessing.cpp
# End Source File
# Begin Source File

SOURCE=.\BatchProcessing.h
# End Source File
# Begin Source File

SOURCE=.\CapturePluginProxy.cpp
# End Source File
# Begin Source File

SOURCE=.\CapturePluginProxy.h
# End Source File
# Begin Source File

SOURCE=.\CapturePort.cpp
# End Source File
# Begin Source File

SOURCE=.\CapturePort.h
# End Source File
# Begin Source File

SOURCE=.\Config.cpp
# End Source File
# Begin Source File

SOURCE=.\Config.h
# End Source File
# Begin Source File

SOURCE=.\ConfigManager.cpp
# End Source File
# Begin Source File

SOURCE=.\ConfigManager.h
# End Source File
# Begin Source File

SOURCE=.\Daemon.cpp
# End Source File
# Begin Source File

SOURCE=.\Daemon.h
# End Source File
# Begin Source File

SOURCE=.\dll.h
# End Source File
# Begin Source File

SOURCE=.\EventStreaming.cpp
# End Source File
# Begin Source File

SOURCE=.\EventStreaming.h
# End Source File
# Begin Source File

SOURCE=.\Filter.cpp
# End Source File
# Begin Source File

SOURCE=.\Filter.h
# End Source File
# Begin Source File

SOURCE=.\g711.c
# End Source File
# Begin Source File

SOURCE=.\g711.h
# End Source File
# Begin Source File

SOURCE=.\ImmediateProcessing.cpp
# End Source File
# Begin Source File

SOURCE=.\ImmediateProcessing.h
# End Source File
# Begin Source File

SOURCE=.\LogManager.cpp
# End Source File
# Begin Source File

SOURCE=.\LogManager.h
# End Source File
# Begin Source File

SOURCE=.\MemUtils.cpp
# End Source File
# Begin Source File

SOURCE=.\MemUtils.h
# End Source File
# Begin Source File

SOURCE=.\MultiThreadedServer.cpp
# End Source File
# Begin Source File

SOURCE=.\MultiThreadedServer.h
# End Source File
# Begin Source File

SOURCE=.\Object.cpp

!IF  "$(CFG)" == "OrkBase - Win32 Release"

!ELSEIF  "$(CFG)" == "OrkBase - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Object.h
# End Source File
# Begin Source File

SOURCE=.\ObjectFactory.cpp

!IF  "$(CFG)" == "OrkBase - Win32 Release"

!ELSEIF  "$(CFG)" == "OrkBase - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ObjectFactory.h
# End Source File
# Begin Source File

SOURCE=.\OrkBase.cpp

!IF  "$(CFG)" == "OrkBase - Win32 Release"

!ELSEIF  "$(CFG)" == "OrkBase - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\OrkBase.h
# End Source File
# Begin Source File

SOURCE=.\OrkClient.cpp
# End Source File
# Begin Source File

SOURCE=.\OrkClient.h
# End Source File
# Begin Source File

SOURCE=.\OrkTrack.cpp
# End Source File
# Begin Source File

SOURCE=.\OrkTrack.h
# End Source File
# Begin Source File

SOURCE=.\PartyFilter.cpp
# End Source File
# Begin Source File

SOURCE=.\Reporting.cpp
# End Source File
# Begin Source File

SOURCE=.\Reporting.h
# End Source File
# Begin Source File

SOURCE=.\SocketStreamer.cpp
# End Source File
# Begin Source File

SOURCE=.\StdString.h
# End Source File
# Begin Source File

SOURCE=.\TapeFileNaming.cpp
# End Source File
# Begin Source File

SOURCE=.\TapeFileNaming.h
# End Source File
# Begin Source File

SOURCE=.\TapeProcessor.cpp
# End Source File
# Begin Source File

SOURCE=.\TapeProcessor.h
# End Source File
# Begin Source File

SOURCE=.\ThreadSafeQueue.h
# End Source File
# Begin Source File

SOURCE=.\Utils.cpp
# End Source File
# Begin Source File

SOURCE=.\Utils.h
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\PartyFilter.h
# End Source File
# Begin Source File

SOURCE=.\SocketStreamer.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
