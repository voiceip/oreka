; orkweb.nsi
;--------------------------------

!include "TextReplace.nsh"
!include "LogicLib.nsh"
;!include "StrFunc.nsh"
;!include "FileFunc.nsh"
!include MUI.nsh
!include "MUI2.nsh"
!include x64.nsh

; The name of the installer
Name "OrkWeb"

; The file to write
OutFile "orkweb-win32-installer.exe"

; The default installation directory
InstallDir $PROGRAMFILES\OrkWeb

ShowInstDetails show

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\orkweb" "Install_Dir"

Function .onInit
  InitPluginsDir
  File /oname=$PLUGINSDIR\verify-java-page.ini verify-java-page.ini
  File /oname=$PLUGINSDIR\verify-tomcat-page.ini verify-tomcat-page.ini
  File /oname=$PLUGINSDIR\verify-mysql-page.ini verify-mysql-page.ini
  ; Get the OS version (32bit versus 64bit) and set registry view accordingly
  ${If} ${RunningX64}
    ; MessageBox MB_OK "running on x64"
    SetRegView 64
  ${EndIf}

FunctionEnd

;---------------------------------
; Function to replace backslashes by slashes
;
; Push $filenamestring (e.g. 'c:\this\and\that\filename.htm')
; Push "\"
; Call StrSlash
; Pop $R0
; ;Now $R0 contains 'c:/this/and/that/filename.htm'
Function StrSlash
  Exch $R3 ; $R3 = needle ("\" or "/")
  Exch
  Exch $R1 ; $R1 = String to replacement in (haystack)
  Push $R2 ; Replaced haystack
  Push $R4 ; $R4 = not $R3 ("/" or "\")
  Push $R6
  Push $R7 ; Scratch reg
  StrCpy $R2 ""
  StrLen $R6 $R1
  StrCpy $R4 "\"
  StrCmp $R3 "/" loop
  StrCpy $R4 "/"  
loop:
  StrCpy $R7 $R1 1
  StrCpy $R1 $R1 $R6 1
  StrCmp $R7 $R3 found
  StrCpy $R2 "$R2$R7"
  StrCmp $R1 "" done loop
found:
  StrCpy $R2 "$R2$R4"
  StrCmp $R1 "" done loop
done:
  StrCpy $R3 $R2
  Pop $R7
  Pop $R6
  Pop $R4
  Pop $R2
  Pop $R1
  Exch $R3
FunctionEnd

;---------------------------------
Function DirState
	!define DirState `!insertmacro DirStateCall`
 
	!macro DirStateCall _PATH _RESULT
		Push `${_PATH}`
		Call DirState
		Pop ${_RESULT}
	!macroend
 
	Exch $0
	Push $1
	ClearErrors
 
	FindFirst $1 $0 '$0\*.*'
	IfErrors 0 +3
	StrCpy $0 -1
	goto end
	StrCmp $0 '.' 0 +4
	FindNext $1 $0
	StrCmp $0 '..' 0 +2
	FindNext $1 $0
	FindClose $1
	IfErrors 0 +3
	StrCpy $0 0
	goto end
	StrCpy $0 1
 
	end:
	Pop $1
	Exch $0
FunctionEnd

;--------------------------------
!define StrStr "!insertmacro StrStr"
 
!macro StrStr ResultVar String SubString
  Push `${String}`
  Push `${SubString}`
  Call StrStr
  Pop `${ResultVar}`
!macroend
 
Function StrStr

  ;Get input from user
  Exch $R0
  Exch
  Exch $R1
  Push $R2
  Push $R3
  Push $R4
  Push $R5
 
  ;Get "String" and "SubString" length
  StrLen $R2 $R0
  StrLen $R3 $R1
  ;Start "StartCharPos" counter
  StrCpy $R4 0
 
  ;Loop until "SubString" is found or "String" reaches its end
  ${Do}
    ;Remove everything before and after the searched part ("TempStr")
    StrCpy $R5 $R1 $R2 $R4
 
    ;Compare "TempStr" with "SubString"
    ${IfThen} $R5 == $R0 ${|} ${ExitDo} ${|}
    ;If not "SubString", this could be "String"'s end
    ${IfThen} $R4 >= $R3 ${|} ${ExitDo} ${|}
    ;If not, continue the loop
    IntOp $R4 $R4 + 1
  ${Loop}
 
  ;Remove part before "SubString" on "String" (if there has one)
  StrCpy $R0 $R1 `` $R4
 
  ;Return output to user
  Pop $R5
  Pop $R4
  Pop $R3
  Pop $R2
  Pop $R1
  Exch $R0
FunctionEnd

;--------------------------------
Function IsTomcatRoot
  Pop $0
  ;DetailPrint "IsTomcatRoot: $0"
  
  ${DirState} "$0\bin" $R0
  StrCmp $R0 "1" 0 notroot
  ;DetailPrint "??? $0"
  ${DirState} "$0\conf" $R0
  StrCmp $R0 "1" 0 notroot
  ${DirState} "$0\webapps" $R0
  StrCmp $R0 "1" 0 notroot
  
  Push "yes"
  Return
notroot:
  Push "no"
FunctionEnd

;--------------------------------
Function FindFiles
  Exch $R5 # callback function
  Exch 
  Exch $R4 # file name
  Exch 2
  Exch $R0 # directory
  Push $R1
  Push $R2
  Push $R3
  Push $R6
 
  Push $R0 # first dir to search
 
  StrCpy $R3 1
 
  nextDir:
    Pop $R0
    IntOp $R3 $R3 - 1
    ClearErrors
    FindFirst $R1 $R2 "$R0\*.*"
    nextFile:
      StrCmp $R2 "." gotoNextFile
      StrCmp $R2 ".." gotoNextFile
 
      ;StrCmp $R2 $R4 0 isDir
        Push "$R0\$R2"
        Call $R5
        Pop $R6
        StrCmp $R6 "stop" 0 isDir
          loop:
            StrCmp $R3 0 done
            Pop $R0
            IntOp $R3 $R3 - 1
            Goto loop
 
      isDir:
        IfFileExists "$R0\$R2\*.*" 0 gotoNextFile
          IntOp $R3 $R3 + 1
          Push "$R0\$R2"
 
  gotoNextFile:
    FindNext $R1 $R2
    IfErrors 0 nextFile
 
  done:
    FindClose $R1
    StrCmp $R3 0 0 nextDir
 
  Pop $R6
  Pop $R3
  Pop $R2
  Pop $R1
  Pop $R0
  Pop $R5
  Pop $R4
FunctionEnd

 
!macro CallFindFiles DIR FILE CBFUNC
Push "${DIR}"
Push "${FILE}"
Push $0
GetFunctionAddress $0 "${CBFUNC}"
Exch $0
Call FindFiles
!macroend

Function FindFilesCallback
  Pop $0
  ;DetailPrint "~~~ $0"
  !insertmacro StrStr $1 $0 "Tomcat"
  StrCmp $1 "" notroot 0
	Push $0
	call IsTomcatRoot
	Pop $1
	;DetailPrint "====== $0"
	StrCmp $1 "yes" 0 notroot
		;DetailPrint "++ $0"
		; $9 represents the Tomcat root folder
		StrCpy $9 $0
		Push "stop"
  notroot:
        Return	
FunctionEnd

;-----------------------------------------
## Displays the username and password dialog
Function UserPassPageShow

 !insertmacro MUI_HEADER_TEXT "Enter Database Username && Password" "If you installed MySQL 5.x with no root password, just click next. To change DB settings later (DB type, hostname, ...), please edit database.hbm.xml"

 PassDialog::InitDialog /NOUNLOAD UserPass /USERTEXT  "DB username" "root" 0
  Pop $R0 # Page HWND

  GetDlgItem $R1 $R0 ${IDC_USERNAME}
  SetCtlColors $R1 0xFF0000 0xFFFFFF
  GetDlgItem $R1 $R0 ${IDC_PASSWORD}
  SetCtlColors $R1 0x0000FF 0xFFFFFF

 PassDialog::Show

FunctionEnd

;-----------------------------------------
 ## Validate username and password
Function UserPassPageLeave

## Pop password & username from stack
Pop $R0
Pop $R1

; Check for empty username
StrCmp $R0 "" Bad Good

Bad:
MessageBox MB_OK|MB_ICONEXCLAMATION "Database username cannot be empty"
Abort

Good:
# $7 contains the DB username
StrCpy $7 $R0
# $8 contains the DB password
StrCpy $8 $R1

FunctionEnd

Function ComponentsPageShow

 ## Disable the Back button
 GetDlgItem $R0 $HWNDPARENT 3
 EnableWindow $R0 0

FunctionEnd

Function VerifyJavaPageShow
  ; Display the Java Install Options dialog
  Push $R0
  InstallOptions::dialog $PLUGINSDIR\verify-java-page.ini
  Pop $R0

FunctionEnd

Function VerifyJavaPageValidate
  ; At this point the user has pressed Next
  ReadINIStr $0 "$PLUGINSDIR\verify-java-page.ini" "Field 2" "State"
  StrCmp $0 0 done  ;Install JRE 7.0.27 if box is checked
    SetOutPath $INSTDIR\Prerequisites
    File ".\Prerequisites\jre-7u2-windows-i586.exe"
    ExecWait '"$INSTDIR\Prerequisites\jre-7u2-windows-i586.exe" /v"/passive /log ./JREsetup.logs"'
  done:
FunctionEnd

Function VerifyTomcatPageShow
  ; Display the Tomcat Install Options dialog
  Push $R0
  
  ; Read Tomcat Path from registry
  ReadRegStr $9 HKLM "SOFTWARE\Apache Software Foundation\Tomcat\7.0\Tomcat7"  "InstallPath"   ; get Tomcat path from Registry
  IfErrors 0 updateDefaultTomcatPath
  StrCpy $9 ""   ; no path found in registry, default to none
  
updateDefaultTomcatPath:
  WriteINIStr "$PLUGINSDIR\verify-tomcat-page.ini" 'Field 4' 'State' $9
  InstallOptions::dialog $PLUGINSDIR\verify-tomcat-page.ini
  Pop $R0

FunctionEnd

Function VerifyTomcatPageValidate

  ; At this point the user has pressed Next

  ReadINIStr $0 "$PLUGINSDIR\verify-tomcat-page.ini" "Field 2" "State"
  StrCmp $0 0 noInstall  ;Install Apache Tomcat 7.0.27 if box is checked
    SetOutPath $INSTDIR\Prerequisites
    File ".\Prerequisites\apache-tomcat-7.0.27.exe"
    ExecWait "$INSTDIR\Prerequisites\apache-tomcat-7.0.27.exe"
    SetOutPath $INSTDIR

  configTomcat:
    ; Replace Tomcat server.xml with one that has port 8443 open for secure access. This file is tomcat-version dependent.
    ; Also install secure key in OrecX/.keystore
    File ".\server.xml"
    File ".\catalina.properties"
    File ".\.keystore"
    ReadRegStr $9 HKLM "SOFTWARE\Apache Software Foundation\Tomcat\7.0\Tomcat7"  "InstallPath"   ; get Tomcat path from Registry
    IfFileExists $9 0 invalidTomcatFolder
    
    ; Update tomcat registry to allocate a min and max of 512MB for Java
    File ".\Tomcat.reg"
    ExecWait 'regedt32.exe /S "$INSTDIR\Tomcat.reg"'
    ; Update tomcat JVM options with -Dfile.encoding=UTF-8
    ExecWait '"$9\bin\tomcat7" //US//Tomcat7 ++JvmOptions=-Dfile.encoding=UTF-8'

    IfFileExists $9\conf\server.xml.ori copyCatalinaProperties 0
        CopyFiles $9\conf\server.xml $9\conf\server.xml.ori
    copyCatalinaProperties:
        IfFileExists $9\conf\catalina.properties.ori copyTomcatConfFiles 0
           CopyFiles $9\conf\catalina.properties $9\conf\catalina.properties.ori
    copyTomcatConfFiles:
        CopyFiles $INSTDIR\server.xml $9\conf   ;replace server.xml but keep a copy of original one.
        CopyFiles $INSTDIR\catalina.properties $9\conf   ;replace catalina.properties but keep a copy of original one.
        CreateDirectory $9\OrecX
        CopyFiles $INSTDIR\.keystore $9\OrecX   ;install key for secure access.
        Delete $INSTDIR\server.xml
        Goto done

  noInstall:
    ReadINIStr $0 "$PLUGINSDIR\verify-tomcat-page.ini" "Field 4" "State"
    StrCpy $9 $0  ; $9 becomes the global variable holding the path to Tomcat
    Push $0
    ; MessageBox MB_ICONEXCLAMATION|MB_OK $R0
    call IsTomcatRoot
    Pop $1
    StrCmp $1 "yes" done notroot
    notroot:
      MessageBox MB_ICONEXCLAMATION|MB_OK "Please enter a valid Tomcat Root Directory."
      Abort
    invalidTomcatFolder:
      MessageBox MB_ICONEXCLAMATION|MB_OK "Could not find Tomcat folder.  Tomcat configuration failed."
      Abort
  done:
    ; Make Tomcat service auto-restart
    Exec "sc config Tomcat7 start= auto"
FunctionEnd

Function VerifyMySQLPageShow
  ; Display the MySQL Install Options dialog
  Push $R0
  InstallOptions::dialog $PLUGINSDIR\verify-mysql-page.ini
  Pop $R0

FunctionEnd

Function VerifyMySQLPageValidate
  ; At this point the user has pressed Next
  ReadINIStr $0 "$PLUGINSDIR\verify-mysql-page.ini" "Field 3" "State"
  StrCmp $0 1 done  ;Checkbox must be checked
    MessageBox MB_ICONEXCLAMATION|MB_OK "Please check when you have installed MySQL!"
    Abort
  done:
FunctionEnd
;--------------------------------
; Pages
!insertmacro MUI_PAGE_LICENSE ..\..\License.txt
Page components
Page directory
Page Custom VerifyJavaPageShow VerifyJavaPageValidate
Page Custom VerifyTomcatPageShow VerifyTomcatPageValidate
Page Custom VerifyMySQLPageShow VerifyMySQLPageValidate
Page Custom UserPassPageShow UserPassPageLeave
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

;---------------------------------
## Control ID's
!define IDC_USERNAME 1215
!define IDC_PASSWORD 1214

## Languages
!insertmacro MUI_LANGUAGE English


;--------------------------------
; The stuff to install
Section "OrkWeb (required)"
SectionIn 1 RO

	!insertmacro MUI_HEADER_TEXT "Installing files" "The installer is now installing the required files"
	
writeHibernateConfig:
	
	SetOutPath $INSTDIR
	File ..\..\LICENSE.txt
	
	IfFileExists $INSTDIR\database.hbm.xml writeLoggingProperties
	file "database.hbm.xml"
	textreplace::ReplaceInFile "$INSTDIR\database.hbm.xml" "$INSTDIR\database.hbm.xml" ">root<" ">$7<" .r0
	textreplace::ReplaceInFile "$INSTDIR\database.hbm.xml" "$INSTDIR\database.hbm.xml" ">password<" ">$8<" .r0
	
writeLoggingProperties:
	IfFileExists $INSTDIR\logging.properties writeOthers
	file "logging.properties"
	; log4j does not like backslashes in properties files, replace them with forward slashes
	Push $INSTDIR
	Push "\"
	Call StrSlash
	Pop $R0
	textreplace::ReplaceInFile "$INSTDIR\logging.properties" "$INSTDIR\logging.properties" "c:/orkweb.log" "$R0/orkweb.log" .r0
        textreplace::ReplaceInFile "$INSTDIR\logging.properties" "$INSTDIR\logging.properties" "c:/orklicense.log" "$R0/orklicense.log" .r0

writeOthers:
	CreateDirectory "c:\oreka\audio"
	
writeExamples:
	file "database-example.hbm.xml"
	file "logging-example.properties"
	textreplace::ReplaceInFile "$INSTDIR\logging-example.properties" "$INSTDIR\logging-example.properties" "c:/orkweb.log" "$INSTDIR/orkweb.log" .r0
	
writeOrkTrackConfig:
	file "orktrack.config.xml"
	
writeApplications:
	DetailPrint "Tomcat folder: $9"
	SetOutPath $9\webapps
	
deleteOrkWeb:	
	ClearErrors
	IfFileExists  $9\webapps\orkweb 0 writeOrkWeb
	CopyFiles $9\webapps\orkweb $TEMP\orkweb
	RMDir /r $9\webapps\orkweb
	DetailPrint "Moving old orkweb tree to $TEMP"

	IfErrors 0 writeOrkWeb
	MessageBox MB_OK "Could not overwrite OrkWeb Tomcat webapps files- Please stop Tomcat before running this installer - Exiting"
	quit
	
writeOrkWeb:
	file /r orkweb
	textreplace::ReplaceInFile "$9\webapps\orkweb\WEB-INF\web.xml" "$9\webapps\orkweb\WEB-INF\web.xml" "c:/oreka/" "$INSTDIR/" .r0
        textreplace::ReplaceInFile "$9\webapps\orkweb\WEB-INF\web.xml" "$9\webapps\orkweb\WEB-INF\web.xml" \
                                   "c:/Program Files/Apache Software Foundation/Tomcat 7.0" \
                                   "$9" .r0
	
deleteOrkTrack:
	ClearErrors
	IfFileExists  $9\webapps\orktrack 0 writeOrkTrack
	CopyFiles $9\webapps\orktrack $TEMP\orktrack
	RMDir /r $9\webapps\orktrack
	DetailPrint "Moving old orktrack tree to $TEMP"

	IfErrors 0 writeOrkTrack
	MessageBox MB_OK "Could not overwrite OrkTrack Tomcat webapps files- Please stop Tomcat before running this installer - Exiting"
	quit	
	
writeOrkTrack:
	file /r orktrack
	textreplace::ReplaceInFile "$9\webapps\orktrack\WEB-INF\web.xml" "$9\webapps\orktrack\WEB-INF\web.xml" "c:/oreka/" "$INSTDIR/" .r0
	textreplace::ReplaceInFile "$9\webapps\orkweb\WEB-INF\web.xml" "$9\webapps\orkweb\WEB-INF\web.xml" \
                                   "c:/Program Files/Apache Software Foundation/Tomcat 7.0" \
                                   "$9" .r0
deleteSharedLib:
	ClearErrors
	IfFileExists $9\shared\lib 0 writeSharedLib
	CopyFiles $9\shared\lib $TEMP\tomcatSharedLib
	RMDir /r $9\shared\lib
	DetailPrint "Moving Tomcat shared libraries to $TEMP"

	IfErrors 0 writeSharedLib
	MessageBox MB_OK "Could not overwrite Tomcat shared libraries - Please stop Tomcat before running this installer - Exiting"
	quit

writeSharedLib:
	SetOverwrite off
	SetOutPath $9\shared\lib
	file lib\*.jar	
	SetOverwrite on

writeRegistryKeys:
	; Write the installation path into the registry
	WriteRegStr HKLM SOFTWARE\orkweb "Install_Dir" "$INSTDIR"
	WriteRegStr HKLM SOFTWARE\orkweb "Tomcat_Dir" "$9"
	
	; Write the uninstall keys for Windows
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\orkweb" "DisplayName" "orkweb"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\orkweb" "UninstallString" '"$INSTDIR\uninstall.exe"'
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\orkweb" "NoModify" 1
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\orkweb" "NoRepair" 1
	WriteUninstaller "uninstall.exe"

SectionEnd

;-------------------------------
; Optional section (can be disabled by the user)
Section "Start Menu Shortcuts"

  CreateDirectory "$SMPROGRAMS\OrkWeb"
  CreateShortCut "$SMPROGRAMS\OrkWeb\Database Configuration.lnk" "$INSTDIR\database.hbm.xml" "" "$INSTDIR\database.hbm.xml" 0  
  CreateShortCut "$SMPROGRAMS\OrkWeb\OrkWeb Install Directory.lnk" "$INSTDIR\" "" "$INSTDIR\" 0
  CreateShortCut "$SMPROGRAMS\OrkWeb\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  
SectionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"
  
  MessageBox MB_OK "Please stop Tomcat before proceeding"
  
  ; Remove Tomcat applications
  ClearErrors
  ReadRegStr $1 HKLM SOFTWARE\orkweb "Tomcat_Dir"
  IfErrors removeRegistryKeys
  RMDir /r "$1\webapps\orkweb"
  RMDir /r "$1\webapps\orktrack"  
  
removeRegistryKeys:  
  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\orkweb"
  DeleteRegKey HKLM SOFTWARE\orkweb

  ; Remove files
  RMDir /r "$INSTDIR\"
  
  ; Remove shortcuts, if any
  RMDir /r "$SMPROGRAMS\orkweb"

SectionEnd
