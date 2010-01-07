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

<p>These Centos/Redhat/Fedora binaries have been generated on a CentOS 4.2 system and consist of a few RPM files including dependencies that are not part of the CentOS distribution. You can install the package using the following commands:</p>
<ul>
<li>yum install boost-devel
<li>yum install libpcap</li>
<li>rpm -i xercesc-2.7.0-1.i386.rpm</li>
<li>rpm -i ACE-5.4.8-1.i386.rpm</li>
<li>rpm -i log4cxx-0.9.7-1.i386.rpm</li>
<li>rpm -i libsndfile-1.0.13-1.i386.rpm</li>
<li>orkbasecxx-0.5-1.i386.rpm</li>
<li>orkaudio-0.5-1.i386.rpm</li>
</ul>

<h3><?=_("Recommended Download")?></h3>
<ul>
  <li><p><a href="http://prdownloads.sourceforge.net/oreka/orkaudio-0.5-313-centos-4.2-i386-RPMs.tar?download">orkaudio-0.5-313-centos-4.2-i386-RPMs.tar</a></p></li>
</ul>

<h3><?=_("System Requirements")?></h3>
<ul>
  <li><?=_("Second NIC (Ethernet port) if you need to monitor an external link for VoIP.")?></li>
  <li><?=_("Mono or multichannel sound card for sound device recording.")?></li>
</ul>


<?php
  include "../include/footer.inc.php";
?>
