
Welcome to Oreka, an open media capture and retrieval platform

Copyright (C) 2005, orecx LLC	http://www.orecx.com

This program is free software, distributed under the terms of
the GNU General Public License.

This package is a binary distribution of the oreka audio recording service. Source code and documentation can be found at http://sourceforge.net/projects/oreka/

--------
Features

* VoIP recording capability by sniffing on a network device (default configuration)
* Works as well on a dedicated server for LAN-wide recording as on an individual PC for personal VoIP recording
* Audio device recording (by default, records all devices in the system) using the Sound Device plugin
* Generates calls details records with local party, remote party and call direction.

The VoIP plugin supports the following protocols:
	- Bidirectional SIP sessions recording with SIP metadata extraction
	- Bidirectional Cisco skinny (aka SCCP) session recording with Skinny metadata extraction
	- Bidirectional Raw RTP session recording with limited metadata extraction (when SIP and Skinny both fail - e.g for H.323)

--------------
Capturing from the right network device

If the software does not seem to produce any audio output, you might be monitoring from the wrong network device. Go to Start/Programs/Orkaudio/Orkaudio Logfile and try to locate lines similar to the following:

....
2005-11-19 11:13:04,638  INFO packet:479 - Available pcap devices:
2005-11-19 11:13:04,638  INFO packet:483 - 	* Realtek RTL8139/810x Family Fast Ethernet NIC                                    (Microsoft's Packet Scheduler)  \Device\NPF_{E0E496FA-DABF-47C1-97C2-DD914DFD3354}
2005-11-19 11:13:04,638  INFO packet:483 - 	* Intel(R) PRO/Wireless 2200BG Network Connection (Microsoft's Packet Scheduler)  \Device\NPF_{2FEDB9F0-F584-48A8-B164-117965F80986}
....

Pick the device you want to monitor, e.g. in this case the wireless connection, copy the Windows device name in the clipboard and paste this into the orkaudio configuration file (config.xml) so that it looks like the following:

....
<Device>\Device\NPF_{2FEDB9F0-F584-48A8-B164-117965F80986}</Device>
....

--------------
Where to tap ?

* If you record from a single PC, no need to worry, just install the software
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

