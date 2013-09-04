<?php
/*
 * Copyright 2004 Matt Brubeck
 * This file is licensed under a Creative Commons license:
 * http://creativecommons.org/licenses/by/2.0/
 */
  require_once "main.inc.php";
  require_once "../latest/versions.inc.php";
  $pageId = "windows";
  $pageTitle = _("OrkAudio Windows Installer");
  include "../include/header.inc.php";
?>

<h2><?=$pageTitle?></h2>

<p>The OrkAudio windows installer is the easiest way to deploy the audio recording service. It will automatically 
install OrkAudio as an NT service and provide start menu links to the audio recordings and the recording details. 
For info on installation and configuration, please check the <a href="../oreka-user-manual.html">online user documentation</a></p>

<h3><?=_("Recommended Download")?></h3>
<ul>
  <li><p><a href="http://downloads.sourceforge.net/project/oreka/oreka/oreka-1.7/orkaudio/orkaudio-1.7-844-os-win32-vc9-installer.zip?r=&ts=1378325812&use_mirror=hivelocity">orkaudio-1.7-844-os-win32-vc9-installer.zip</a></p></li>
</ul>

<h3><?=_("System Requirements")?></h3>
<ul>
  <li><?=_("Windows XP or later.")?></li>
  <li><?=_("Second NIC (Ethernet port) if you need to port mirror an external link for VoIP.")?></li>
</ul>


<?php
  include "../include/footer.inc.php";
?>
