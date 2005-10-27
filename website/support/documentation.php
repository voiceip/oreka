<?php
/*
 * Copyright 2004 Matt Brubeck
 * This file is licensed under a Creative Commons license:
 * http://creativecommons.org/licenses/by/2.0/
 */
  require_once "main.inc.php";
  $pageId = "documentation";
  $pageTitle = _("Documentation");
  include "../include/header.inc.php";

	echo "<h2>$pageTitle</h2>";

	// i18n-hint:  Please add a note that the manuals below are in English.  If
	// there is documentation available in your language, link to it here.
	echo "<p>"._('This page links to the Oreka documentation in various forms:')."</p>";
?>

<ul>
	<li><a href="../oreka-documentation.html">HTML, one page</a></li>
	<li><a href="../oreka-documentation-multi">HTML, one page per section</a></li>
</ul>

<?php
  include "../include/footer.inc.php";
?>
