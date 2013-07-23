<?php
/*
 * Copyright 2005 Matt Brubeck
 * This file is licensed under a Creative Commons license:
 * http://creativecommons.org/licenses/by/2.0/
 */
  require_once "main.inc.php";
  $sectionId = "";
  $pageTitle = _("Open source voice recording system");
  include "include/header.inc.php";
  include "include/news.inc.php";
  include "latest/versions.inc.php";
  include "include/detect-os.inc.php";

?>
<div id="about">
  <h2><?=_("The Open Source Enterprise Telephony Recording and Retrieval System")?></h2>
  <div id="screenshot">
    <!-- TODO: Auto-select or randomly rotate screenshot? -->
  </div>
  <p><?=_('Oreka is an enterprise telephony recording and retrieval system with web based user interface. The project currently supports recording voice from VoIP SIP, Cisco Skinny (aka SCCP), raw RTP and audio sound device and runs on multiple operating systems and database systems.')?></p>
  <p><?=_('It can record audio from most PBX and telephony systems such as BroadWorks, Metaswitch, Asterisk, FreeSwitch, OpenSIPS, Avaya, Nortel, Mitel, Siemens, Cisco Call Manager, Cosmocom, NEC, etc...')?></p>
  <p><?=_('It is amongst others being used in Call Centers and Contact Centers for Quality monitoring (QM) purposes.')?></p>

  <p><?=_('Oreka is sponsored by OrecX LLC. Professional open source support as well as commercial versions are available <a href="http://www.orecx.com">here</a>')?></p>
</div>

<div id="download">
  <h3><a href="download">Download Oreka</a></h3>
</div>

<div id="news">
  <?php
    global $news_items;
    foreach ($news_items as $item) {
      $dateStr = $item->dateStr();
      ?>
      <div class="newsitem">
        <h3><?="$dateStr: $item->title"?></h3>
        <?=$item->body?>
      </div>
      <?php
    }
  ?>
  <h4><a href="about/news"><?=_("More news items...")?></a></h4>
</div>

<?php
  include "include/footer.inc.php";
?>
