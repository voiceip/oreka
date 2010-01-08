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

<p><font color="red">Note: OrkAudio was not re-compiled for this platform since the 0.5 version.</font></p>

<p>These Debian/ubuntu binaries have been generated on a Ubuntu Hoary system and consist of three deb files. You can install them using the following commands:</p>
<ul>
<li>dpkg -i log4cxx-0.9.7_0.9.7-1_i386.deb</li>
<li>dpkg -i orkbasecxx_X.X-1_i386.deb</li>
<li>dpkg -i orkaudio_X.X-1_i386.deb</li>
</ul>
<p>
OrkAudio depends on some third party packages. You can install them using the following commands. Some of those are not part of the core distribution so you will need to enable the apt-get "universe" source by uncommenting the right lines in /etc/apt/sources.list.
</p>
<ul>
<li>apt-get install libace-dev</li>
<li>apt-get install libboost-dev</li>
<li>apt-get install liblog4cpp-dev</li>
<li>apt-get install libpcap0.7-dev</li>
<li>apt-get install libxerces26-dev</li>
<li>apt-get install libsndfile1-dev</li>
</ul>

<h3><?=_("Recommended Download")?></h3>
<ul>
  <li><p><a href="http://prdownloads.sourceforge.net/oreka/orkaudio-0.5-313-ubuntu-5.04-i386-DEBs.tar?download">orkaudio-0.5-313-ubuntu-5.04-i386-DEBs.tar</a></p></li>
</ul>

<h3><?=_("System Requirements")?></h3>
<ul>
  <li><?=_("Second NIC (Ethernet port) if you need to monitor an external link for VoIP.")?></li>
  <li><?=_("Mono or multichannel sound card for sound device recording.")?></li>
</ul>


<?php
  include "../include/footer.inc.php";
?>
