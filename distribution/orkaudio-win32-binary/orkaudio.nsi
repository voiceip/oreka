; orkaudio.nsi
;--------------------------------

; The name of the installer
Name "Orkaudio"

; The file to write
OutFile "OrkaudioInstaller.exe"

; The default installation directory
InstallDir $PROGRAMFILES\Orkaudio

ShowInstDetails show

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\Orkaudio" "Install_Dir"

;--------------------------------

; Pages

Page components
Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

;--------------------------------
; The stuff to install
Section "Orkaudio (required)"

  SectionIn RO
  
  ; Stop orkaudio service if earlier version is running
  nsSCM::Stop orkaudio
    ; wait for the service to stop
  sleep 4000
  
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  
  ; Files

file "OrkAudio.exe"
file "config.xml"
file "logging.properties"
file "OrkBase.dll"
file "orkaudio.log"
file "tapelist.log"
file "WinPcap_3_1.exe"
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
file "audiocaptureplugins\Generator.dll"
file "audiocaptureplugins\SoundDevice.dll"
file "audiocaptureplugins\VoIp.dll"  
  
  SetOutPath $INSTDIR\AudioRecordings
  
  nsSCM::Install orkaudio orkaudio 16 2 "$INSTDIR\orkaudio.exe" "" "" "" ""
  Pop $0
  StrCmp $0 "success" serviceOk  
	MessageBox MB_OK "Orkaudio NT Service installation failed - service probably existing before running this installer"
  serviceOk:
  
  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\Orkaudio "Install_Dir" "$INSTDIR"
  
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Orkaudio" "DisplayName" "Orkaudio"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Orkaudio" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Orkaudio" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Orkaudio" "NoRepair" 1
  WriteUninstaller "uninstall.exe"
  
SectionEnd

; Optional section (can be disabled by the user)
Section "Start Menu Shortcuts"

  CreateDirectory "$SMPROGRAMS\Orkaudio"
  CreateShortCut "$SMPROGRAMS\Orkaudio\Orkaudio Recordings.lnk" "$INSTDIR\AudioRecordings" "" "$INSTDIR\AudioRecordings" 0
  CreateShortCut "$SMPROGRAMS\Orkaudio\Orkaudio Recordings List.lnk" "$INSTDIR\tapelist.log" "" "$INSTDIR\tapelist.log" 0  
  CreateShortCut "$SMPROGRAMS\Orkaudio\Orkaudio Logfile.lnk" "$INSTDIR\orkaudio.log" "" "$INSTDIR\orkaudio.log" 0  
  CreateShortCut "$SMPROGRAMS\Orkaudio\Orkaudio Install Directory.lnk" "$INSTDIR\" "" "$INSTDIR\" 0
  CreateShortCut "$SMPROGRAMS\Orkaudio\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  
SectionEnd

Section "Install WinPcap 3.1"
	ExecWait "WinPcap_3_1.exe"
SectionEnd

Section "Run orkaudio NT service"
  nsSCM::Start orkaudio
  Pop $0
  StrCmp $0 "success" startOk  
	MessageBox MB_OK "Orkaudio NT Service start failed"
  startOk:
SectionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"
  
  ; stop service
  nsSCM::Stop orkaudio
  Pop $0
  StrCmp $0 "success" stopOk  
	MessageBox MB_OK "Orkaudio NT Service stop failed"
  stopOk:
  ; wait for the service to stop
  sleep 4000
  ;  uninstall service
  nsSCM::Remove orkaudio
  Pop $0
  StrCmp $0 "success" uninstallOk  
	MessageBox MB_OK "Orkaudio NT Service uninstallation failed - service has probably been removed earlier"
  uninstallOk:
  
  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Orkaudio"
  DeleteRegKey HKLM SOFTWARE\Orkaudio

  ; Remove files
  Delete "$INSTDIR\audiocaptureplugins\*.*"  
  RMDir "$INSTDIR\audiocaptureplugins"
  Delete "$INSTDIR\*.*"
  RMDir "$INSTDIR"
  
  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\Orkaudio\*.*"
  RMDir "$SMPROGRAMS\Orkaudio"


SectionEnd
