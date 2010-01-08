<?php
/*
 * Copyright 2004 Matt Brubeck
 * This file is licensed under a Creative Commons license:
 * http://creativecommons.org/licenses/by/2.0/
 */
  require_once "main.inc.php";
  $pageId = "links";
  $pageTitle = _("Links");
  include "../include/header.inc.php";
?>

<h2><?=$pageTitle?></h2>

<h3>Related projects</h3>
<ul>
<li><a href="http://www.enderunix.org/voipong/">voipong</a> is a linux only RTP session capture software.</li>
<li><a href="http://vomit.xtdnet.nl/">vomit</a> is a tool for extracting Cisco VoIP audio from a network dump.</li>
</ul>

<h3><?=_("Related Software Categories at sourceforge.net")?></h3>
<ul>
  <li><a href="http://sourceforge.net/softwaremap/trove_list.php?form_cat=115"><?=_("Audio Capture/Recording")?></a>
  <li><a href="http://sourceforge.net/softwaremap/trove_list.php?form_cat=247"><?=_("Telephony")?></a>
</ul>

<h3>Open source libraries</h3>
<p>Oreka is build upon various open source libraries. Please refer to the <a href="credits">credits</a> page</p>
<?php
  include "../include/footer.inc.php";
?>
