
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

-------------
Running orkaudio

* run the daemon:

root@myhost:/ # orkaudio

* Or run attached to tty:

root@myhost:/ # orkaudio debug

-------------
Files location

* Configuration files are in /etc/orkaudio
* Logging output is in /var/log/orkaudio/orkaudio.log
* Captured audio files are in /var/log/orkaudio/[year]/[month]/[day]/[hour]
* Call details records can be found in /var/log/orkaudio/tapelist.log

--------------
Capturing from the right network device

If the software does not seem to produce any audio output, you might be monitoring from the wrong network device. edit /var/log/orkaudio/orkaudio.log and try to locate lines similar to the following:

....
2005-11-21 21:57:14,253  INFO packet:479 - Available pcap devices:
2005-11-21 21:57:14,254  INFO packet:483 -      *  eth0
2005-11-21 21:57:14,254  INFO packet:483 -      * Pseudo-device that captures on all interfaces any
2005-11-21 21:57:14,255  INFO packet:483 -      *  lo

....

Pick the device you want to monitor, e.g. eth0  and paste this into the orkaudio configuration file (/etc/orkaudio/config.xml) so that it looks like the following:

....
<Device>eth0</Device>
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


