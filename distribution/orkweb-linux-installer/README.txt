Creating the installer:

* Put the desired orktrack.war and orkweb.war in this directory
* Put the content of {tomcat}/shared/lib as a zip archive in this directory. The zip archive needs to be called lib.zip and must be generated so that all jar files are in a directory called 'lib' when extracted.

* tar cvf archive.tar install.pl database.hbm.xml logging.properties tomcat apache-tomcat-5.5.20.tar.gz jre-6u16-linux-i586.bin.gz lib.zip orkweb.war orktrack.war server.xml .keystore
* cat install-header archive.tar > orkweb-1.2-667-os-linux-installer.sh
