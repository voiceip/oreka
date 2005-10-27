<?php
/*
 * Copyright 2003 Dominic Mazzoni
 * Copyright 2005 Matt Brubeck
 * This file is licensed under a Creative Commons license:
 * http://creativecommons.org/licenses/by/2.0/
 */
  require_once "main.inc.php";
  $pageId = "privacy";
  $pageTitle = _("Privacy Policy");
  include "../include/header.inc.php";
?>

<h2><?=$pageTitle?></h2>

<!-- Translators - please add a sentence which explains that these
 announcements will usually be in English only.  Possibly people can
 join users lists in their native languages later on, to hear
 announcements there.
-->
<h3 id="announce"><?=_("Announcement List Policy")?></h3>

<?=_('<p>You can subscribe to our announcement list by entering your address in the form on the <a href="../">Audacity home page</a>.  We will use this list only to send you brief announcements of new Audacity releases.  We will never share the addresses on this list with anyone.</p>
<p>To <b>unsubscribe</b> from the list at any time, enter your address in the form on our <a href="../">home page</a> and press the "Remove" button.  If you have any problems or questions, please contact <a href="mailto:dominic@audacityteam.org">dominic@audacityteam.org</a>.</p>')?>

<h3 id="lists"><?=_("Mailing List Policy")?></h3>

<?=_('<p>(This applies to audacity-help, audacity-users, and other <a href="../contact/lists">discussion lists</a>.)</p>
<p>These are public mailing lists.  When you send a message to any of these addresses, including <a href="../contact/">audacity-help</a>, it will be forwarded to dozens of people, including Audacity developers and others.  SourceForge and other web sites may publish archives of these lists.  <b>Your email address will normally be removed from published archives for your privacy, and the Audacity developers will not share your address with anyone or add it to any lists.</b>  However, we can\'t prevent other subscribers from seeing or publishing your messages.</p>
<p>If you have any questions about this policy or are not comfortable writing to a public mailing list, send a private e-mail to the lead Audacity developer at: <a href="mailto:dominic@audacityteam.org">dominic@audacityteam.org</a>.</p>')?>

<?php
  include "../include/footer.inc.php";
?>

