<?php
/*
 * Copyright 2004 Matt Brubeck
 * This file is licensed under a Creative Commons license:
 * http://creativecommons.org/licenses/by/2.0/
 */
  require_once "main.inc.php";
  require_once "../latest/versions.inc.php";
  $pageId = "windows";
  $pageTitle = _("Windows");
  include "../include/header.inc.php";
?>

<h2><?=$pageTitle?></h2>

<p>The windows binary distribution of OrkAudio is not yet an installer package. However installation is straighforward and described in README.txt in the root folder of the package.</p>

<h3><?=_("Recommended Download")?></h3>
<ul>
  <li><p><a href="http://sourceforge.net/project/showfiles.php?group_id=150919&package_id=166774">orkaudio-0.1-win32-binary.zip</a></p></li>
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
