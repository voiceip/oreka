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

echo _('<h3>Recording and storage</h3>
<ul>
<li>Record VoIP SIP sessions by passively listening to network packets. Both sides of a conversation are mixed together and each call is logged as a separate audio file.</li>
<li>Record from a standard sound device (e.g. microphone or line input). Can record multiple channels at the same time. Each recording goes to separate audio files</li>
<li>Open plugin architecture for audio capture means that the system is potentially capable of recording from any audio source.</li>
<li>Automatic audio segmentation so that continuous audio sources can be split in separate audio files and easily retrieved later.</li>
<li>Voice activity detection.</li>
<li>GSM6.10, A-Law and u-Law compression available in order to save disk space.</li>
<li>Recording metadata logged to file and/or any mainstream database system.</li>
</ul>');

  echo _('<h3>User interface</h3>
<p>Recordings retrieval can be done using the following criteria (when available):</p>
<ul>
<li>Timestamp</li>
<li>Recording duration</li>
<li>Direction (for a telephone call)</li>
<li>Remote Party (for a telephone call)</li>
<li>Local Party (for a telephone call)</li>
</ul>');

  include "../include/footer.inc.php";
?>
