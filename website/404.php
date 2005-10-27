<?php
/*
 * Copyright 2005 Matt Brubeck
 * This file is licensed under a Creative Commons license:
 * http://creativecommons.org/licenses/by/2.0/
 */
  require_once "main.inc.php";
  $sitePath = "/.";
  $pageTitle = _("Page Not Found");
  include "include/header.inc.php";
?>

<h2><?=$pageTitle?></h2>
<p><?=_('Sorry, the page you are looking for was not found.  Please try the <a href="/">Oreka home page</a> or the <a href="/site-map">site map</a>.')?></p>

<?php
  include "include/footer.inc.php";
?>
