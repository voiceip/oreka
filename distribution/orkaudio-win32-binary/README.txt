
Welcome to Oreka, an open media capture and retrieval platform

Copyright (C) 2005, orecx LLC	http://www.orecx.com

This program is free software, distributed under the terms of
the GNU General Public License.

This package is a binary distribution of the oreka audio recording service. Source code and documentation can be found at http://sourceforge.net/projects/oreka/

===========
Quick start

--------
Features

* VoIP recording capability by sniffing on a network device (default configuration)
* Audio device recording (by default, records all devices in the system) using the Sound Device plugin
* Generates "tape" (recordings) details records

The VoIP plugin supports the following protocols:
	- Bidirectional SIP sessions recording with SIP metadata extraction
	- Bidirectional Cisco skinny (aka SCCP) session recording with Skinny metadata extraction
	- Bidirectional Raw RTP session recording with limited metadata extraction (when SIP and Skinny both fail)

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

* For VoIP recording on an entire LAN, you need to tap an ethernet link carrying all 
the voice traffic you want to monitor. There are three methods:
1. Ehternet tap
2. Ethernet Hub
3. Switch SPAN port
For more info on those options, see:
http://www.snort.org/docs and more specifically:
http://www.snort.org/docs/iss-placement.pdf

----------------
How to configure

* General configuration is found in config.xml
* Logging configuration is found in logging.properties 

-------------------------------------------------
Where to find generated "tapes" and "tape detail records"

* Point the windows file explorer to the OrkAudio install directory. Files are stored in subdirectories using the following path scheme:  YYYY\MM\DD\HH\

* "tape details records" can be found in tapelist.log

