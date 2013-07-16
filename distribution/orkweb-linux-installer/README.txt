Creating the installer:

* Put the desired orktrack.war and orkweb.war in this directory
* Put the content of {tomcat}/shared/lib as a zip archive in this directory. The zip archive needs to be called lib.zip and must be generated so that all jar files are in a directory called 'lib' when extracted.
* Make sure the tomcat and java (JRE) that are needed are also in the directory

For 32-bit OS:
--------------
* tar cvf archive.tar install.pl database.hbm.xml logging.properties tomcat lib.zip orkweb.war orktrack.war server.xml catalina.properties .keystore apache-tomcat-7.0.23.tar.gz jre-7u2-linux-i586.tar.gz 
* cat install-header archive.tar > orkweb-1.7-831-os-linux-installer.sh
* chmod +x orkweb-1.7-831-os-linux-installer.sh 
* tar -cvf orkweb-1.7-831-os-linux-installer.sh.tar orkweb-1.7-830-os-linux-installer.sh

For 64-bit OS:
--------------
* tar cvf archive.tar install.pl database.hbm.xml logging.properties tomcat lib.zip orkweb.war orktrack.war server.xml catalina.properties .keystore apache-tomcat-7.0.23.tar.gz jre-7u4-linux-x64.tar.gz
* cat install-header archive.tar > orkweb-1.7-831-x64-os-linux-installer.sh
* chmod +x orkweb-1.7-831-x64-os-linux-installer.sh 
* tar -cvf orkweb-1.7-831-x64-os-linux-installer.sh.tar orkweb-1.7-830-x64-os-linux-installer.sh
