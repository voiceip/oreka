<?php
/*
 * Copyright 2005 Matt Brubeck
 * This file is licensed under a Creative Commons license:
 * http://creativecommons.org/licenses/by/2.0/
 */
  require_once "main.inc.php";
  $sectionId = "";
  $pageTitle = _("Audio streams recording and retrieval");
  include "include/header.inc.php";
  include "include/news.inc.php";
  include "latest/versions.inc.php";
  include "include/detect-os.inc.php";

?>
<div id="about">
  <h2><?=_("The open source, cross-platform audio stream recording and retrieval system")?></h2>
  <div id="screenshot">
    <!-- TODO: Auto-select or randomly rotate screenshot? -->
  </div>
  <p><?=_('Oreka is a modular and cross-platform system for recording and retrieval of audio streams. The project currently supports VoIP and sound device based capture. Recordings metadata can be stored in any mainstream database. Retrieval of captured sessions is web based.')?></p>
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
