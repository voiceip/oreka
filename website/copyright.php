<?php
/*
 * Copyright 2004 Matt Brubeck
 * This file is licensed under a Creative Commons license:
 * http://creativecommons.org/licenses/by/2.0/
 */
  require_once "main.inc.php";
  $pageTitle = _("Web Site Copyright and Trademark");
  include "include/header.inc.php";
?>

<h2><?=$pageTitle?></h2>

<?php
  $thisYear = date("Y");

  // i18n-hint: The Creative Commons web page below is available in several
  // languages.  Please link to the version in your language if possible.
	//
	// You may add a sentence like "Czech translation copyright YOUR NAME HERE"
	// if you wish.
  echo "<p>".sprintf(_('This web site is Copyright &copy; %s members of the Audacity development team.  Except where otherwise noted, all text and images on this site are licensed under the <a href="http://creativecommons.org/licenses/by/2.0/">Creative Commons Attribution License, version 2.0</a>.  You may modify, copy, distribute, and display this material, but you must give credit to the original authors.  Please see the license for details.'), $thisYear)."</p>";


  include "include/footer.inc.php";
?>
