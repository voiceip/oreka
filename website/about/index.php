<?php
/*
 * Copyright 2004 Matt Brubeck
 * This file is licensed under a Creative Commons license:
 * http://creativecommons.org/licenses/by/2.0/
 */
  require_once "main.inc.php";
  $pageId = "";
  $pageTitle = _("About Oreka");
  include "../include/header.inc.php";

  echo "<h2>$pageTitle</h2>";
?>

  <p><?=_('Oreka is an enterprise telephony recording and retrieval system with web based user interface. The project currently supports recording voice from VoIP SIP, Cisco Skinny (aka SCCP), raw RTP and audio sound device and runs on multiple operating systems and database systems.')?></p>
  <p><?=_('It can record audio from most PBX and telephony systems such as BroadWorks, Metaswitch, Asterisk, FreeSwitch, OpenSIPS, Avaya, Nortel, Mitel, Siemens, Cisco Call Manager, Cosmocom, NEC, etc...')?></p>
  <p><?=_('It is amongst others being used in Call Centers and Contact Centers for Quality monitoring (QM) purposes.')?></p>

  <p><?=_('Oreka is sponsored by OrecX LLC. Professional open source support as well as commercial versions are available <a href="http://www.orecx.com">here</a>')?></p>

<?php
  // i18n-hint: You may change the link addresses below, if there is a
  // version of the page in your language at a different location.
  //
  // For official translations of the phrase "free software", please see:
  //   http://www.fsf.org/philosophy/fs-translations.html
  //
  // Also check the translations of this web page:
  //   http://www.fsf.org/philosophy/free-sw.html

  include "../include/footer.inc.php";
?>
