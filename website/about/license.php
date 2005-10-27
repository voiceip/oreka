<?php
/*
 * Copyright 2004 Matt Brubeck
 * This file is licensed under a Creative Commons license:
 * http://creativecommons.org/licenses/by/2.0/
 */
  require_once "main.inc.php";
  $pageId = "license";
  $pageTitle = _("License");
  include "../include/header.inc.php";
?>

<h2><?=$pageTitle?></h2>
<p>Copyright (C) 2005, orecx LLC
<a href="http://www.orecx.com">http://www.orecx.com</a></p>
<?php

// i18n-hint:  These gnu.org web pages are available in several languages.
// Please link to the version in your language, if possible.  For example:
//
//   http://www.gnu.org/licenses/licenses.es.html#GPL
//   http://www.gnu.org/licenses/gpl-faq.cs.html
//
// See the bottom of each page for versions in other languages.
//
echo _('<p>Oreka is free software; you can redistribute it and/or modify it under the terms of the <a href="http://www.gnu.org/licenses/licenses.html#GPL">GNU General Public License</a> as published by the Free Software Foundation.</p>

<p>See also: <a href="http://www.gnu.org/licenses/gpl-faq.html">Frequently Asked Questions about the GNU GPL</a>.</p>');

?>

<!-- TODO: Copyright and licenses for libraries. -->

<?php
  include "../include/footer.inc.php";
?>
