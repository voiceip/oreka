<?php
/*
 * Copyright 2004 Matt Brubeck
 * This file is licensed under a Creative Commons license:
 * http://creativecommons.org/licenses/by/2.0/
 */
  require_once "main.inc.php";
  require_once "../latest/versions.inc.php";
  $pageId = "redhat";
  $pageTitle = _("OrkAudio Redhat/CentOS/Fedora Package");
  include "../include/header.inc.php";
?>

<h2><?=$pageTitle?></h2>

<h3><?=_("Recommended Download")?></h3>
<ul>
  <li><p><a href="http://sourceforge.net/projects/oreka/files/oreka/oreka-1.2/orkaudio/orkaudio-1.2-671-os-i386.centos5-installer.sh.tar/download">orkaudio-1.2-671-os-i386.centos5-installer.sh.tar</a></p></li>
</ul>

<h3><?=_("System Requirements")?></h3>
<ul>
  <li><?=_("Second NIC (Ethernet port) if you need to monitor an external link for VoIP.")?></li>
  <li><?=_("Mono or multichannel sound card for sound device recording.")?></li>
</ul>


<?php
  include "../include/footer.inc.php";
?>
