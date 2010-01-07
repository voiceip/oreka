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
?>

<ul>
	<li><a href="../oreka-documentation.html">Developer documentation</a></li>
	<li><a href="../oreka-user-manual.html">User manual</a></li>
</ul>

<?php
  include "../include/footer.inc.php";
?>
