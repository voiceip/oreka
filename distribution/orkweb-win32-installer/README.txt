To build orkweb installer, perform the following steps:

Pre-requisites:
---------------
- Get the version you want to build from the repository
- Build and deploy it to your tomcat environment
- Make sure the $tomcat/shared/lib folder is up-to-date
- Copy the following folders:
	$tomcat/shared/lib to $oreka/distribution/orkweb-win32-installer/lib
	$tomcat/webapps/orktrack to $oreka/distribution/orkweb-win32-installer/orktrack
	$tomcat/webapps/orkweb   to $oreka/distribution/orkweb-win32-installer/orkweb
- Create $oreka/distribution/orkweb-win32-installer/Prerequisites and add to it 
  the Java Run-Time environment JRE 1.6 and Tomcat 5.5.23:
	    jre-6u16-windows-i586-s.exe
	    apache-tomcat-5.5.23.exe

Compile the installer ($oreka/distribution/orkweb-win32-installer):
-------------------------------------------------------------------

Compile orkweb.nsi using a .nsi compiler (recommended IDE: NSI Script Editor.)
The result is the "orkweb-win32-installer.exe"

	


	
