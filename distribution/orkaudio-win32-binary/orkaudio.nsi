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
  
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  
  ; Files
file "ACE-AUTHORS"
file "ACE-COPYING"
file "ACE-README"
file "ACE-THANKS"
file "ACE-VERSION"
file "ACE.dll"
file "LICENSE.txt"
file "OrkAudio.exe"
file "OrkBase.dll"
file "README.txt"
file "VERSION.txt"
file "boost-LICENSE_1_0.txt"
file "boost-README"
file "config.xml"
file "log4cxx-AUTHORS"
file "log4cxx-COPYING"
file "log4cxx-ChangeLog"
file "log4cxx-README"
file "log4cxx-license.apl"
file "log4cxx.dll"
file "logging.properties"
file "messages.log"
file "orkaudio.log"
file "orkaudio.nsi"
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
  CreateShortCut "$SMPROGRAMS\Orkaudio\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  CreateShortCut "$SMPROGRAMS\Orkaudio\Orkaudio.lnk" "$INSTDIR\" "" "$INSTDIR\" 0
  ;CreateShortCut "$SMPROGRAMS\Orkaudio\Orkaudio (MakeNSISW).lnk" "$INSTDIR\orkaudio.nsi" "" "$INSTDIR\orkaudio.nsi" 0
  
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
