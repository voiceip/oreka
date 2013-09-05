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

<style type="text/css">
wrn {color: red;}
</style>

<h2><?=$pageTitle?></h2>

<h3><?=_("Installers")?></h3>

<p>The OrkWeb installers provided below are the easiest way to deploy the database logging (OrkTrack) and web UI (OrkWeb) software to your system.</p> 
<p>These installers provide the option of installing Tomcat 7 and Java JRE 7 on your system, both required by OrkWeb and OrkTrack.  However, before you proceed with the installation, you need to download and install MySQL (or another SQL-based database, see documentation).</p> 
<p>
    <ul>
      <li>Windows installer: <a href="http://sourceforge.net/projects/oreka/files/oreka/oreka-1.7/orkweb/orkweb-1.7-838-os-win32-installer.zip/download">orkweb-1.7-838-os-win32-installer.zip</a></li>
      <li>Linux 64-bit installer (CentOS and RHEL): <a href="http://sourceforge.net/projects/oreka/files/oreka/oreka-1.7/orkweb/orkweb-1.7-838-x64-os-linux-installer.sh.tar/download">orkweb-1.7-838-x64-os-linux-installer.sh.tar</a></li>
    </ul>
</p>

<h3><?=_("Manual Deployment")?></h3>

<p>You can also deploy OrkWeb and OrkTrack manually by downloading the package below and following the instructions in the README.txt file.</p>
<ul>
  <li><p><a href="http://sourceforge.net/projects/oreka/files/oreka/oreka-1.7/orkweb/oreka-1.7-838-orktrack-orkweb.zip/download">oreka-1.7-838-orktrack-orkweb.zip</a></p></li>
</ul>

<p><font color="red"> WARNING: if you are upgrading OrkWeb/OrkTrack from 0.5-311, the database schema has changed.  Please use the database script file: <a href="http://sourceforge.net/projects/oreka/files/oreka/oreka-1.2/orkweb/updateOrekaDB_to_v1.zip/download">updateOrekaDB_to_v1.zip</a>.</font><font>  It includes a README.txt file and a .sql script file with details in the header.</font></p>

<h3><?=_("System Requirements")?></h3>
<ul>
  <li><?=_("MySQL or other supported database (see documentation)")?></li>
  <li><?=_("Java 5.0 (aka 1.5) JRE or JVM, or better.  Recommended: Java 7")?></li>
  <li><?=_("Tomcat 5.5 or better.  Recommended: Tomcat 7")?></li>
</ul>


<?php
  include "../include/footer.inc.php";
?>
