<?php
/*
 * Copyright 2004 Matt Brubeck
 * This file is licensed under a Creative Commons license:
 * http://creativecommons.org/licenses/by/2.0/
 */
  require_once "main.inc.php";
  $pageId = "";
  $pageTitle = _("Download");
  include "../include/header.inc.php";
?>

<h2><?=$pageTitle?></h2>

<p><?=_("The following packages are available for download:")?></p>
<ul>
	<li>OrkAudio audio capture server as a Windows precompiled binary</li>
	<li>Entire Oreka codebase (Cross platform C++ and Java)</li>
	<li>OrkTrack and OrkWeb tracking and user interface servers as precompiled Java5 war files</li>
</ul>

<p><?=_('You may modify and distribute Oreka under the <a href="../about/license">GNU GPL</a>.')?></p>

<?php
  include "../include/footer.inc.php";
?>
