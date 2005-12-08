<?php
/*
 * Copyright 2004 Matt Brubeck
 * This file is licensed under a Creative Commons license:
 * http://creativecommons.org/licenses/by/2.0/
 */
  require_once "main.inc.php";
  require_once "../latest/versions.inc.php";
  $pageId = "debian";
  $pageTitle = _("OrkAudio Debian/Ubuntu Package");
  include "../include/header.inc.php";
?>

<h2><?=$pageTitle?></h2>

<p>This Debian/ubuntu dpkg package has been generated on a Ubuntu Hoary system. It references all necessary dependencies. You can install it using the following command:</p>
<p><b>dpkg -i orkaudio-0.3-linux-i386.deb</b></p>
<p>This will prompt you if you need to install any additional dependencies first. You can install them using the command:</p>
<p><b>apt-get install [package]</b></p>

<h3><?=_("Recommended Download")?></h3>
<ul>
  <li><p><a href="http://prdownloads.sourceforge.net/oreka/orkaudio-0.3-linux-i386.deb?download">orkaudio-0.3-linux-i386.deb</a></p></li>
</ul>

<h3><?=_("System Requirements")?></h3>
<ul>
  <li><?=_("Second NIC (Ethernet port) if you need to monitor an external link for VoIP.")?></li>
  <li><?=_("Mono or multichannel sound card for sound device recording.")?></li>
</ul>


<?php
  include "../include/footer.inc.php";
?>
