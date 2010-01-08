<?php
/*
* Copyright 2004 Matt Brubeck
* This file is licensed under a Creative Commons license:
* http://creativecommons.org/licenses/by/2.0/
*/
require_once "main.inc.php";
$pageId = "features";
$pageTitle = _("Features");
include "../include/header.inc.php";

echo "<h2>$pageTitle</h2>";
echo "<p>"._('Oreka currently has the following features:')."</p>";
?>
<h3>Recording and storage</h3>
<ul>
<li>Record VoIP RTP sessions by passively listening to network packets. Both sides of a conversation are mixed together and each call is logged as a separate audio file. When SIP or Cisco Skinny (SCCP) signalling is detected, the associated metadata is also extracted</li>
<li>Record from a standard sound device (e.g. microphone or line input). Can record multiple channels at the same time. Each recording goes to separate audio files</li>
<li>Open plugin architecture for audio capture means that the system is potentially capable of recording from any audio source</li>
<li>Plugin architecture for codecs or any other signal processing filter</li>
<li>Automatic audio segmentation so that continuous audio sources can be split in separate audio files and easily retrieved later</li>
<li>Capture from multiple Network devices in parallel</li>
<li>Capture from pcap trace files</li>
<li>Voice activity detection</li>
<li>G.711 (A-Law, U-Law), G.722, GSM6.10 and iLBC RTP encodings supported</li>
<li>G.711 (A-Law, U-Law) and GSM6.1 encoded wav files supported as storage formats</li>
<li>Automatic transcoding from wire format to storage format</li>
<li>Recording metadata logged to file and/or any mainstream database system</li>
</ul>

<h3>User interface</h3>
<p>Recordings retrieval can be done using the following criteria (when available):</p>
<ul>
<li>Timestamp</li>
<li>Recording duration</li>
<li>Direction (for a telephone call)</li>
<li>Remote Party (for a telephone call)</li>
<li>Local Party (for a telephone call)</li>
</ul>

<h3>Compatibility</h3>
<p>Oreka has been reported to work on the following platforms and should actually work on many more.</p>
<ul>
<li>Cisco CallManager and CallManager Express v. 3.x, 4.x, 5.x, 6.x and 7.x</li>
<li>SIP</li>
<li>Lucent APX8000</li>
<li>Avaya S8500</li>
<li>Siemens HiPath</li>
<li>VocalData</li>
<li>Sylantro</li>
</ul>
<?
  include "../include/footer.inc.php";
?>
