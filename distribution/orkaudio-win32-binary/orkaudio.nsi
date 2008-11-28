; orkaudio.nsi
;--------------------------------

!include "MUI2.nsh"

; The name of the installer
Name "OrkAudio"

; The file to write
OutFile "OrkAudioInstaller.exe"

; The default installation directory
InstallDir $PROGRAMFILES\OrkAudio

ShowInstDetails show

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\OrkAudio" "Install_Dir"

;--------------------------------

; Pages

!insertmacro MUI_PAGE_LICENSE ..\..\LICENSE.txt
Page components
Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

;--------------------------------
Function GetTime
	!define GetTime `!insertmacro GetTimeCall`
 
	!macro GetTimeCall _FILE _OPTION _R1 _R2 _R3 _R4 _R5 _R6 _R7
		Push `${_FILE}`
		Push `${_OPTION}`
		Call GetTime
		Pop ${_R1}
		Pop ${_R2}
		Pop ${_R3}
		Pop ${_R4}
		Pop ${_R5}
		Pop ${_R6}
		Pop ${_R7}
	!macroend
 
	Exch $1
	Exch
	Exch $0
	Exch
	Push $2
	Push $3
	Push $4
	Push $5
	Push $6
	Push $7
	ClearErrors
 
	StrCmp $1 'L' gettime
	StrCmp $1 'A' getfile
	StrCmp $1 'C' getfile
	StrCmp $1 'M' getfile
	StrCmp $1 'LS' gettime
	StrCmp $1 'AS' getfile
	StrCmp $1 'CS' getfile
	StrCmp $1 'MS' getfile
	goto error
 
	getfile:
	IfFileExists $0 0 error
	System::Call /NOUNLOAD '*(i,l,l,l,i,i,i,i,&t260,&t14) i .r6'
	System::Call /NOUNLOAD 'kernel32::FindFirstFileA(t,i)i(r0,r6) .r2'
	System::Call /NOUNLOAD 'kernel32::FindClose(i)i(r2)'
 
	gettime:
	System::Call /NOUNLOAD '*(&i2,&i2,&i2,&i2,&i2,&i2,&i2,&i2) i .r7'
	StrCmp $1 'L' 0 systemtime
	System::Call /NOUNLOAD 'kernel32::GetLocalTime(i)i(r7)'
	goto convert
	systemtime:
	StrCmp $1 'LS' 0 filetime
	System::Call /NOUNLOAD 'kernel32::GetSystemTime(i)i(r7)'
	goto convert
 
	filetime:
	System::Call /NOUNLOAD '*$6(i,l,l,l,i,i,i,i,&t260,&t14)i(,.r4,.r3,.r2)'
	System::Free /NOUNLOAD $6
	StrCmp $1 'A' 0 +3
	StrCpy $2 $3
	goto tolocal
	StrCmp $1 'C' 0 +3
	StrCpy $2 $4
	goto tolocal
	StrCmp $1 'M' tolocal
 
	StrCmp $1 'AS' tosystem
	StrCmp $1 'CS' 0 +3
	StrCpy $3 $4
	goto tosystem
	StrCmp $1 'MS' 0 +3
	StrCpy $3 $2
	goto tosystem
 
	tolocal:
	System::Call /NOUNLOAD 'kernel32::FileTimeToLocalFileTime(*l,*l)i(r2,.r3)'
	tosystem:
	System::Call /NOUNLOAD 'kernel32::FileTimeToSystemTime(*l,i)i(r3,r7)'
 
	convert:
	System::Call /NOUNLOAD '*$7(&i2,&i2,&i2,&i2,&i2,&i2,&i2,&i2)i(.r5,.r6,.r4,.r0,.r3,.r2,.r1,)'
	System::Free $7
 
	IntCmp $0 9 0 0 +2
	StrCpy $0 '0$0'
	IntCmp $1 9 0 0 +2
	StrCpy $1 '0$1'
	IntCmp $2 9 0 0 +2
	StrCpy $2 '0$2'
	IntCmp $6 9 0 0 +2
	StrCpy $6 '0$6'
 
	StrCmp $4 0 0 +3
	StrCpy $4 Sunday
	goto end
	StrCmp $4 1 0 +3
	StrCpy $4 Monday
	goto end
	StrCmp $4 2 0 +3
	StrCpy $4 Tuesday
	goto end
	StrCmp $4 3 0 +3
	StrCpy $4 Wednesday
	goto end
	StrCmp $4 4 0 +3
	StrCpy $4 Thursday
	goto end
	StrCmp $4 5 0 +3
	StrCpy $4 Friday
	goto end
	StrCmp $4 6 0 error
	StrCpy $4 Saturday
	goto end
 
	error:
	SetErrors
	StrCpy $0 ''
	StrCpy $1 ''
	StrCpy $2 ''
	StrCpy $3 ''
	StrCpy $4 ''
	StrCpy $5 ''
	StrCpy $6 ''
 
	end:
	Pop $7
	Exch $6
	Exch
	Exch $5
	Exch 2
	Exch $4
	Exch 3
	Exch $3
	Exch 4
	Exch $2
	Exch 5
	Exch $1
	Exch 6
	Exch $0
FunctionEnd

;--------------------------------
; The stuff to install
Section "OrkAudio (required)"

  SectionIn RO
  
  ; Stop orkaudio service if earlier version is running
  nsSCM::Stop orkaudio
    ; wait for the service to stop
  sleep 4000
  
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  
; Generate installer generation timestamp 
;   %B represents month name 
;   %b abbreviated month name
!define /date INSTALLER_TIMESTAMP "%Y%m%d_%H%M%S"

; Generate current timestamp
${GetTime} "" "L" $0 $1 $2 $3 $4 $5 $6
; $0="01"      day
; $1="04"      month
; $2="2005"    year
; $3="Friday"  day of week name
; $4="16"      hour
; $5="05"      minute
; $6="50"      seconds


BackupConfigXml:
IfFileExists config.xml 0 BackupLoggingProperties
    CopyFiles "$INSTDIR\config.xml" "$INSTDIR\config.xml.$2$1$0_$4$5$6"

BackupLoggingProperties:
IfFileExists logging.properties 0 InstallAllFiles
    CopyFiles "$INSTDIR\logging.properties" "$INSTDIR\logging.properties.$2$1$0_$4$5$6"

InstallAllFiles:
file "OrkAudio.exe"
file "config.xml"
file "logging.properties"
file "OrkBase.dll"
file "orkaudio.log"
file "tapelist.log"
file "WinPcap_4_0_2.exe"
file "LICENSE.txt"
file "README.txt"
file "VERSION.txt"
file "ACE-AUTHORS"
file "ACE-COPYING"
file "ACE-README"
file "ACE-THANKS"
file "ACE-VERSION"
file "ACE.dll"
file "boost-LICENSE_1_0.txt"
file "boost-README"
file "log4cxx-AUTHORS"
file "log4cxx-COPYING"
file "log4cxx-ChangeLog"
file "log4cxx-README"
file "log4cxx-license.apl"
file "log4cxx.dll"
file "portaudio-LICENSE.txt"
file "portaudio-README.txt"
file "xerces-c-LICENSE"
file "xerces-c-NOTICE"
file "xerces-c-credits.txt"
file "xerces-c-version.incl"
file "xerces-c_2_6.dll"

  SetOutPath $INSTDIR\audiocaptureplugins
file "audiocaptureplugins\VoIp.dll"  

  SetOutPath $INSTDIR\plugins
file "plugins\RtpMixer.dll"  
  
  SetOutPath c:\oreka\audio  
  
  nsSCM::Install orkaudio orkaudio 16 2 "$INSTDIR\orkaudio.exe" "" "" "" ""
  Pop $0
  StrCmp $0 "success" serviceOk  
	MessageBox MB_OK "OrkAudio NT Service installation failed - service probably existing before running this installer"
  serviceOk:
  
  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\OrkAudio "Install_Dir" "$INSTDIR"
  
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OrkAudio" "DisplayName" "OrkAudio"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OrkAudio" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OrkAudio" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OrkAudio" "NoRepair" 1
  WriteUninstaller "uninstall.exe"
  
SectionEnd

; Optional section (can be disabled by the user)
Section "Start Menu Shortcuts"

  CreateDirectory "$SMPROGRAMS\OrkAudio"
  CreateShortCut "$SMPROGRAMS\OrkAudio\OrkAudio Recordings.lnk" "c:\oreka\audio" "" "c:\oreka\audio" 0
  CreateShortCut "$SMPROGRAMS\OrkAudio\OrkAudio Recordings List.lnk" "$INSTDIR\tapelist.log" "" "$INSTDIR\tapelist.log" 0  
  CreateShortCut "$SMPROGRAMS\OrkAudio\OrkAudio Logfile.lnk" "$INSTDIR\orkaudio.log" "" "$INSTDIR\orkaudio.log" 0  
  CreateShortCut "$SMPROGRAMS\OrkAudio\OrkAudio Install Directory.lnk" "$INSTDIR\" "" "$INSTDIR\" 0
  CreateShortCut "$SMPROGRAMS\OrkAudio\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  
SectionEnd

Section "Install WinPcap 4.0.2"
	ExecWait "$INSTDIR\WinPcap_4_0_2.exe"
SectionEnd

Section "Run orkaudio NT service"
  nsSCM::Start orkaudio
  Pop $0
  StrCmp $0 "success" startOk  
	MessageBox MB_OK "OrkAudio NT Service start failed"
  startOk:
SectionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"
  
  ; stop service
  nsSCM::Stop orkaudio
  Pop $0
  StrCmp $0 "success" stopOk  
	MessageBox MB_OK "Could not stop OrkAudio NT Service, maybe it was not running?"
  stopOk:
  ; wait for the service to stop
  sleep 4000
  ;  uninstall service
  nsSCM::Remove orkaudio
  Pop $0
  StrCmp $0 "success" uninstallOk  
	MessageBox MB_OK "OrkAudio NT Service uninstallation failed - OrkAudio NT service has probably been removed earlier"
  uninstallOk:
  
  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OrkAudio"
  DeleteRegKey HKLM SOFTWARE\OrkAudio

  ; Remove files
  Delete "$INSTDIR\audiocaptureplugins\*.*"  
  RMDir "$INSTDIR\audiocaptureplugins"
  Delete "$INSTDIR\plugins\*.*"  
  RMDir "$INSTDIR\plugins"
  Delete "$INSTDIR\*.*"
  RMDir "$INSTDIR"
  
  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\OrkAudio\*.*"
  RMDir "$SMPROGRAMS\OrkAudio"


SectionEnd
