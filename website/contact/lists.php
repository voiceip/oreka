<?php
/*
 * Copyright 2005 Matt Brubeck
 * This file is licensed under a Creative Commons license:
 * http://creativecommons.org/licenses/by/2.0/
 */
  require_once "main.inc.php";
  $pageId = "lists";
  $pageTitle = _("Mailing Lists");
  include "../include/header.inc.php";

  $subscriberOnlyStr = _("You must subscribe to send messages to this list.");
	// i18n-hint: The "%s" will be replaced by the name of a mailing list.
  $archiveStr = _("%s archives and subscription information");

	echo "<h2>$pageTitle</h2>";

	// i18n-hint: Please add a note that these lists are in English.  If there are
	// Audacity mailing lists or forums in your language, please link to them here.
	echo "<p>"._('These mailing lists are for discussion of the Audacity audio editor.  </p>
<p>Note: These are public mailing lists.  For details, see our <a href="privacy">privacy policy</a>.')."</p>";
?>

<dl>
  <dt id="users">audacity-users</dt>
  <dd>
    <p><?=_('Discuss Audacity with other users and developers.')?>  <?=$subscriberOnlyStr?></p>
    <p><a href="http://lists.sourceforge.net/lists/listinfo/audacity-users"><?php printf($archiveStr, "audacity-users")?></a>
  </dd>

  <dt id="translation">audacity-translation</dt>
  <dd>
    <p><?=_('For translators localizing the Audacity software and web site.')?>  <?=$subscriberOnlyStr?></p>
    <p><a href="http://lists.sourceforge.net/lists/listinfo/audacity-translation"><?php printf($archiveStr, "audacity-translation")?></a>
  </dd>

  <dt id="devel">audacity-devel</dt>
  <dd>
    <p><?=_('For developers working with the Audacity source code and documentation.')?>  <?=$subscriberOnlyStr?></p>
    <p><a href="http://lists.sourceforge.net/lists/listinfo/audacity-devel"><?php printf($archiveStr, "audacity-devel")?></a>
  </dd>

  <dt id="nyquist">audacity-nyquist</dt>
  <dd>
    <p><?=_('Share questions and tips about programming <a href="../help/nyquist">Nyquist plug-ins</a>.')?>  <?=$subscriberOnlyStr?></p>
    <p><a href="http://lists.sourceforge.net/lists/listinfo/audacity-nyquist"><?php printf($archiveStr, "audacity-nyquist")?></a>
  </dd>

  <dt id="bugs">audacity-bugs</dt>
  <dd>
    <p><?=_('Developers can subscribe to this mailing list to receive automatic updates from <a href="../community/developers#bugzilla">Bugzilla</a>.')?></p>
    <p><a href="http://lists.sourceforge.net/lists/listinfo/audacity-bugs"><?php printf($archiveStr, "audacity-bugs")?></a>
  </dd>

  <dt id="help">audacity-help</dt>
  <dd>
    <p><?=_('Send a message to this list if you have questions or comments for the Audacity developers.  You do not need to subscribe to this list to send a message.')?></p>
    <p><a href="http://lists.sourceforge.net/lists/listinfo/audacity-help"><?php printf($archiveStr, "audacity-help")?></a>
  </dd>

<?php
  include "../include/footer.inc.php";
?>
