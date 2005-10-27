<?php
/*
 * Copyright 2004 Matt Brubeck
 * This file is licensed under a Creative Commons license:
 * http://creativecommons.org/licenses/by/2.0/
 */
  require_once "main.inc.php";
  $pageId = "";
  $pageTitle = _("Contact Us");
  include "../include/header.inc.php";
?>

<h2><?=$pageTitle?></h2>

<?php
  // i18n-hint:  Please add a note that we can answer questions to
  // audacity-help in English only.  If there is an Audacity forum or mailing
  // list in your language, you may add a link to it below.
  echo _('<p>If you have questions or comments for the Audacity developers, e-mail us at: <a href="mailto:audacity-help@lists.sourceforge.net">audacity-help@lists.sourceforge.net</a>.
(This is a public mailing list.  For details, see our <a href="../contact/privacy">privacy policy</a>.)</p>

<p><b>When you report a bug or problem, please:</b></p>
<ol>
  <li>Before you contact us, check the <a href="../help/faq">Frequently Asked Questions</a>.</li>
  <li>Tell us what version of Audacity and which operating system you are using.</li>
  <li>Include details of what you are trying to do, and any error messages or other problems you experience.</li>
</ol>');

  echo _('<h3>Discussion Lists</h3>
<p>To discuss Audacity with other users and developers, join our <a href="../contact/lists">mailing lists</a>.</p>');

  include "../include/footer.inc.php";
?>
