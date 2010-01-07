<?php
/*
 * Copyright 2004 Matt Brubeck
 * This file is licensed under a Creative Commons license:
 * http://creativecommons.org/licenses/by/2.0/
 */
  require_once "main.inc.php";
  $pageId = "";
  $pageTitle = _("Documentation and Support");
  include "../include/header.inc.php";

  echo "<h2>$pageTitle</h2>";

  // i18n-hint: If there are any special resources for Audacity users in your
  // language (forums, documentation, mailing lists...), you can add links to
  // them here.
  echo _('<p>If you have a question, check the list of <a href="faq">Frequently Asked Questions</a> before asking on the <a href="lists">mailing lists</a>.</p>
<p>For instructions on how to build, install and use Oreka, see the <a href="documentation">documentation</a></p>');
?>
<p>Commercial support and add-ons are available from <a href="http://www.orecx.com">orecx LLC</a>, the primary sponsor and developer of Oreka.</p>
<?php
  include "../include/footer.inc.php";
?>
