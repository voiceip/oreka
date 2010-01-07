<?php
/*
 * Copyright 2004 Matt Brubeck
 * This file is licensed under a Creative Commons license:
 * http://creativecommons.org/licenses/by/2.0/
 */
  require_once "main.inc.php";
  require_once "../latest/versions.inc.php";
  $pageId = "war";
  $pageTitle = _("OrkWeb/OrkTrack");
  include "../include/header.inc.php";
?>

<h2><?=$pageTitle?></h2>

<p>This package contains all you need to deploy database logging (orktrack) and web UI (orkweb) to a Tomcat server. Installing instructions can be found in README.txt.
<b>Warning: Java 5.0 (aka 1.5) required</b></p>

<h3><?=_("Recommended Download")?></h3>
<ul>
  <li><p><a href="http://prdownloads.sourceforge.net/oreka/oreka-0.5-311-orktrack-orkweb.zip?download">oreka-0.5-311-orktrack-orkweb.zip</a></p></li>
</ul>

<h3><?=_("System Requirements")?></h3>
<ul>
  <li><?=_("MySQL or other supported database (see documentation)")?></li>
  <li><?=_("Java 5.0 (aka 1.5) JRE or JVM")?></li>
  <li><?=_("Tomcat 5.5 or better")?></li>
</ul>


<?php
  include "../include/footer.inc.php";
?>
