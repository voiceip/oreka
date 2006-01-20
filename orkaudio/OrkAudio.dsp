# Microsoft Developer Studio Project File - Name="OrkAudio" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=OrkAudio - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "OrkAudio.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "OrkAudio.mak" CFG="OrkAudio - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "OrkAudio - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "OrkAudio - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "OrkAudio - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /Zi /O2 /Op /Ob0 /I "." /I "..\OrkBaseCxx" /I "C:\devExt\boost\boost_1_32_0\\" /I "C:\devExt\ACE_wrappers" /I "C:\devExt\log4cxx\log4cxx-0.9.7\include" /I "C:\devExt\xerces++\xerces-c_2_6_0-windows_nt-msvc_60\include" /I "C:\devExt\libsndfile\src" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 OrkBase.lib LIBSNDFILE.lib xerces-c_2.lib log4cxx.lib ace.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /libpath:"..\OrkbaseCxx\Release" /libpath:"C:\devExt\ACE_wrappers\lib" /libpath:"C:\devExt\log4cxx\log4cxx-0.9.7\msvc\Lib\Release" /libpath:"C:\devExt\xerces++\xerces-c_2_6_0-windows_nt-msvc_60\lib" /libpath:"C:\devExt\libsndfile\Release"

!ELSEIF  "$(CFG)" == "OrkAudio - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "." /I "..\OrkBaseCxx" /I "C:\devExt\boost\boost_1_32_0\\" /I "C:\devExt\ACE_wrappers" /I "C:\devExt\log4cxx\log4cxx-0.9.7\include" /I "C:\devExt\xerces++\xerces-c_2_6_0-windows_nt-msvc_60\include" /I "C:\devExt\libsndfile\src" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR /FD /GZ /c
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 OrkBaseD.lib LIBSNDFILE.lib xerces-c_2D.lib log4cxx.lib aced.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept /libpath:"..\OrkbaseCxx\Debug" /libpath:"C:\devExt\ACE_wrappers\lib" /libpath:"C:\devExt\log4cxx\log4cxx-0.9.7\msvc\Lib\Debug" /libpath:"C:\devExt\xerces++\xerces-c_2_6_0-windows_nt-msvc_60\lib" /libpath:"C:\devExt\libsndfile\Debug"

!ENDIF 

# Begin Target

# Name "OrkAudio - Win32 Release"
# Name "OrkAudio - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "Messages"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Messages\CaptureMsg.cpp
# End Source File
# Begin Source File

SOURCE=.\Messages\CaptureMsg.h
# End Source File
# Begin Source File

SOURCE=.\Messages\DeleteTapeMsg.cpp
# End Source File
# Begin Source File

SOURCE=.\Messages\DeleteTapeMsg.h
# End Source File
# Begin Source File

SOURCE=.\Messages\PingMsg.cpp
# End Source File
# Begin Source File

SOURCE=.\Messages\PingMsg.h
# End Source File
# Begin Source File

SOURCE=.\Messages\TapeMsg.cpp
# End Source File
# Begin Source File

SOURCE=.\Messages\TapeMsg.h
# End Source File
# Begin Source File

SOURCE=.\messages\TestMsg.cpp
# End Source File
# Begin Source File

SOURCE=.\messages\TestMsg.h
# End Source File
# End Group
# Begin Group "AudioFile"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\AudioFile\AudioFile.cpp
# End Source File
# Begin Source File

SOURCE=.\AudioFile\AudioFile.h
# End Source File
# Begin Source File

SOURCE=.\AudioFile\LibSndFileFile.cpp
# End Source File
# Begin Source File

SOURCE=.\AudioFile\LibSndFileFile.h
# End Source File
# Begin Source File

SOURCE=.\audiofile\MediaChunkFile.cpp
# End Source File
# Begin Source File

SOURCE=.\audiofile\MediaChunkFile.h
# End Source File
# Begin Source File

SOURCE=.\AudioFile\PcmFile.cpp
# End Source File
# Begin Source File

SOURCE=.\AudioFile\PcmFile.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\App.h
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

SOURCE=.\MultiThreadedServer.cpp
# End Source File
# Begin Source File

SOURCE=.\MultiThreadedServer.h
# End Source File
# Begin Source File

SOURCE=.\OrkAudio.cpp
# End Source File
# Begin Source File

SOURCE=.\OrkAudio.h
# End Source File
# Begin Source File

SOURCE=.\Reporting.cpp
# End Source File
# Begin Source File

SOURCE=.\Reporting.h
# End Source File
# Begin Source File

SOURCE=.\ThreadSafeQueue.h
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
