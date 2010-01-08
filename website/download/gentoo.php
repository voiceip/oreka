<?php
/*
 * Copyright 2004 Matt Brubeck
 * This file is licensed under a Creative Commons license:
 * http://creativecommons.org/licenses/by/2.0/
 */
  require_once "main.inc.php";
  require_once "../latest/versions.inc.php";
  $pageId = "gentoo";
  $pageTitle = _("OrkAudio Gentoo ebuild");
  include "../include/header.inc.php";
?>

<h2><?=$pageTitle?></h2>

<p><font color="red">Note: OrkAudio was not re-compiled for this platform since the 0.5 version.</font></p>

<p>Mike Auty has created a Gentoo ebuild that can be found here: <a href="http://bugs.gentoo.org/show_bug.cgi?id=123390">[bugs.gentoo.org]</a></p>

<?php
  include "../include/footer.inc.php";
?>
