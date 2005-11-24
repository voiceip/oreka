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

<p>The source code (Java and C++) is known to compile under Linux and Windows. Build instructions can be found in README.txt in the root folder of the package. This project requires a few dependencies that are listed in README.txt. For Windows developers, there is a convenience package that includes all open source libraries (precompiled) that this project is depending upon.<p>

<h3><?=_("Recommended Download")?></h3>

<ul>
  <li><p>Windows zip file: <a href="http://www.sourceforge.net/projects/oreka/download">oreka-0.3.zip</a></p></li>
  <li><p>Unix tarball: <a href="http://www.sourceforge.net/projects/oreka/download">oreka-0.3.tar.gz</a></p></li>
</ul>

<h3>Optional download</h3>
<ul>
	<li>Windows C++ dependencies package: <a href="http://www.sourceforge.net/projects/oreka/download">oreka-0.3-cxx-win32-dependencies.zip</a></li>
	<li>Java dependencies package: <a href="http://www.sourceforge.net/projects/oreka/download">oreka-0.3-java-dependencies.zip</a></li>
</ul>

<?php
  include "../include/footer.inc.php";
?>
