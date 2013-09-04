<?php
/*
 * Copyright 2004 Matt Brubeck
 * This file is licensed under a Creative Commons license:
 * http://creativecommons.org/licenses/by/2.0/
 */
  require_once "main.inc.php";
  require_once "../latest/versions.inc.php";
  $pageId = "source";
  $pageTitle = _("Source Code");
  include "../include/header.inc.php";
?>

<h2><?=$pageTitle?></h2>

<p>The source code (Java and C++) is known to compile under Linux, Windows and FreeBSD. Build instructions can be found in the <a href="../oreka-documentation.html">documentation</a>.<p>

<h3><?=_("Getting the source code")?></h3>

<p>Use subversion to checkout the latest source code:</p>
<p><i>svn checkout svn://svn.code.sf.net/p/oreka/svn/trunk oreka-svn</i></p>

<h3>Optional download</h3>
<ul>
	<li>Java dependencies package: <a href="http://sourceforge.net/projects/oreka/files/oreka/oreka-1.7-838/orkweb/oreka-1.7-838-java-dependencies.zip/download">oreka-1.7-838-java-dependencies.zip</a></li>
</ul>

<?php
  include "../include/footer.inc.php";
?>
