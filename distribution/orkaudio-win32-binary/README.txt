
Welcome to Oreka, an open media capture and retrieval platform

Copyright (C) 2005, orecx LLC	http://www.orecx.com

This program is free software, distributed under the terms of
the GNU General Public License.

This package is a binary distribution of the oreka audio recording service. Source code and documentation can be found at http://sourceforge.net/projects/oreka/

===========
Quick start

--------
Features

* Bidirectional SIP sessions recording using the VoIP plugin (default configuration)
* Audio device recording (by default, records all devices in the system) using the Sound Device plugin
* Generates "tape" (recordings) details record

--------------
How to install

* Download and install winpcap 3.1 (choose the windows installer version). This is a network packet capture library. 
	http://www.winpcap.org

* Unzip this package in a folder of your choice.

* Open a cmd box, navigate to the install directory

* To run on the command line, open a cmd box, navigate to the install directory and issue the following command:   

	c:\oreka\orkaudio> OrkAudio.exe debug

* To run as a service, issue the following command:

	c:\oreka\orkaudio> OrkAudio.exe install

* To uninstall service, issue the following command:

	c:\oreka\orkaudio> OrkAudio.exe uninstall

----------------
How to configure

* General configuration is found in config.xml
* Logging configuration is found in logging.properties

-------------------------------------------------
Where to find "tapes" and "tapes" details records

* Point the windows file explorer to the OrkAudio install directory. Files are stored in subdirectories using the following path scheme:  YYYY\MM\DD\HH\

* "tapes" details records can be found in tapelist.log

