<?php
/*
 * Copyright 2004 Matt Brubeck
 * This file is licensed under a Creative Commons license:
 * http://creativecommons.org/licenses/by/2.0/
 */
  require_once "main.inc.php";
  $pageId = "release-notes";
  $pageTitle = _("Release Notes");
  include "../include/header.inc.php";
  include "../latest/versions.inc.php";
?>

<h2><?=$pageTitle?></h2>

<p>2008-11-28 Oreka 1.0</p>
<ul>
<li>Enhanced Interactive intelligence support</li>
<li>VoIP plugin now capable of recording Sangoma TDM boards via wanpipe RTP tap</li>
<li>Added stereo recording capability. Currently limited to G.711 storage format.</li>
<li>RTP out-of-band DTMF digits can now be extracted</li>
<li>Audio file naming now configurable</li>
<li>Added Wifi support</li>
<li>Added arbitrary SIP field extraction feature</li>
<li>IAX2 native Asterisk signalling is now supported</li>
</ul>

<p>2006-07-27 Oreka 0.5-313</p>
<ul>
<li>Added supports Cisco CallManager 5 signalling</li>
<li>Added IP address and address range filtering capability</li>
<li>Added support for MPLS and 802.Q VLAN traffic</li>
<li>Local Party now always reported as local IP address if emtpy</li>
<li>Added local and remote IP addresses reporting</li>
<li>Improved SIP detection</li>
<li>Improved Cisco Skinny detection</li>
<li>Configurable queue sizes for high load systems</li>
<li>Added ability to replay an entire directory worth of pcap traces</li>
<li>Recording duration now more precise</li>
<li>Fixed a number of problems that could lead to one sided, garbled or saturated recordings</li>
<li>Fixed memory leak</li>
<li>Redundant RTP packets now detected and discarded</li>
<li>Improved robustness with more packet sanity checking</li>
</ul>

<p>2006-02-16 Oreka 0.5</p>
<ul>
<li>Fixed a critical bug that could cause Orkaudio to crash given a certain sequence of RTP packets</li>
<li>Fixed a SIP detection issue on Siemens platform (Siemens Optipoint 400)</li>
</ul>

<p>2006-02-12 Oreka 0.4</p>
<ul>
<li>Oreka can now monitor multiple Network devices in parallel</li>
<li>Oreka can now replay tcpdump (pcap) network trace files</li>
<li>Improved RTP detection (Before: some RTP streams could go undetected)</li>
<li>Improved Cisco Skinny signalling management (Before: Sessions could break up spuriously and single session could be seen as two separate sessions)</li>
<li>Improved SIP detection (Before: SIP messages on some platforms went undetected)</li>
<li>Improved SIP signalling management (Before: Sessions could break up spuriously)</li>
<li>Oreka can now store in "pcmwav" format, ie, pcm stored in a wav file without compression</li>
<li>Sample rate can now be different than 8KHz across Oreka</li>
<li>Sample rate now tunable in SoundDevice plugin</li>
<li>Codecs can now be plugins</li>
</ul>



<p>2005-11-24 Oreka 0.3</p>
<ul>
<li>Windows installer now available</li>
<li>"make install" target now works under linux</li>
<li>Debian/Ubuntu binary package now available</li>
<li>Audio output location can now be configured ("AudioOutputPath" tag in config.xml)</li>
<li>Orkaudio now daemonizes by default under linux</li>
<li>Fixed bug where Skinny session could go undetected</li>
<li>Fixed bug where NT service sometimes crashed when stopped by the user</li>
</ul>


<p>2005-11-02 Oreka 0.2</p>
<ul>
<li>VoIp plugin now supports Cisco skinny (aka SCCP) protocol (including metadata extraction)</li>
<li>VoIp plugin now supports Raw RTP sessions recording.</li>
<li>VoIP plugin now tries to associate a SIP call to a capture port that is made of the actual local SIP agent (phone) IP address and TCP port. Before that, a capture port could be made of a gateway or external IP address and TCP port.</li>
<li>Default to the last OS returned network device when config file does not specify or specifies invalid network device.</li>
<li>Visual studio project now opens without errors.</li>
<li>Fixed win32 crash when network device identifier in config file was invalid.</li>
</ul>
<p>2005-10-20 Oreka 0.1</p>
<ul>
<li>This is the first release of this software.</li>
</ul>
<?php
  include "../include/footer.inc.php";
?>
