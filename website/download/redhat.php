<?php
/*
 * Copyright 2004 Matt Brubeck
 * This file is licensed under a Creative Commons license:
 * http://creativecommons.org/licenses/by/2.0/
 */
  require_once "main.inc.php";
  require_once "../latest/versions.inc.php";
  $pageId = "redhat";
  $pageTitle = _("OrkAudio RHEL/CentOS Installer");
  include "../include/header.inc.php";
?>

<h2><?=$pageTitle?></h2>

<p>The OrkAudio CentOS/RHEL installer is the easiest way to deploy the audio recording component. It will automatically 
install OrkAudio as a service.
For info on installation and configuration, please check the <a href="../oreka-user-manual.html">online user documentation</a></p>

<h3><?=_("Recommended Download")?></h3>
<ul>
  <li><p><a href="http://downloads.sourceforge.net/project/oreka/oreka/oreka-1.7/orkaudio/orkaudio-1.7-844-os-x86_64.centos6-installer.sh.tar">orkaudio-1.7-844-os-x86_64.centos6-installer.sh.tar</a></p></li>
</ul>

<h3><?=_("System Requirements")?></h3>
<ul>
  <li><?=_("CentOS or RHEL 6 - 64 bit, minimal install (Graphical environment not necessary).")?></li>
  <li><?=_("Second NIC (Ethernet port) if you need to port mirror an external link for VoIP.")?></li>
</ul>


<?php
  include "../include/footer.inc.php";
?>
