<?php
/*
 * Copyright 2004 Matt Brubeck
 * This file is licensed under a Creative Commons license:
 * http://creativecommons.org/licenses/by/2.0/
 */
  require_once "main.inc.php";
  require_once "../latest/versions.inc.php";
  $pageId = "windows";
  $pageTitle = _("OrkAudio Windows");
  include "../include/header.inc.php";
?>

<h2><?=$pageTitle?></h2>

<p>The OrkAudio windows installer is the easiest way to deploy the audio recording service. It will automatically install OrkAudio as an NT service and provide start menu links to the audio recordings and the recording details. For info on configuring, please check the README.txt in the install folder and the <a href="../support/documentation">online documentation</a></p>

<h3><?=_("Recommended Download")?></h3>
<ul>
  <li><p><a href="http://sourceforge.net/projects/oreka/files/oreka/oreka-1.2/orkaudio/orkaudio-1.2-673-os-win32-installer.zip/download">orkaudio-1.2-673-os-win32-installer.zip</a></p></li>
</ul>

<h3><?=_("System Requirements")?></h3>
<ul>
  <li><?=_("Windows 2000, XP, or later.")?></li>
  <li><?=_("Second NIC (Ethernet port) if you need to monitor an external link for VoIP.")?></li>
  <li><?=_("Mono or multichannel sound card for sound device recording.")?></li>
</ul>


<?php
  include "../include/footer.inc.php";
?>
