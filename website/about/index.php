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

  echo _('<p>Oreka is a modular and cross-platform system for recording and retrieval of audio streams. The project currently supports VoIP and sound device based capture. Recordings metadata can be stored in any mainstream database. Retrieval of captured sessions is web based. For more details, please refer to the <a href="features">features</a> page</p>

');

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
