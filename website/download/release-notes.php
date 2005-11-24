<?php
/*
 * Copyright 2004 Matt Brubeck
 * This file is licensed under a Creative Commons license:
 * http://creativecommons.org/licenses/by/2.0/
 */
  require_once "main.inc.php";
  $pageId = "release-notes";
  $pageTitle = _("Release Notes");
  include "../include/header.inc.php";
  include "../latest/versions.inc.php";
?>

<h2><?=$pageTitle?></h2>

<p>2005-11-24 Oreka 0.3</p>
<ul>
<li>Windows installer now available</li>
<li>"make install" target now works under linux</li>
<li>Debian/Ubuntu binary package now available</li>
<li>Audio output location can now be configured ("AudioOutputPath" tag in config.xml)</li>
<li>Orkaudio now daemonizes by default under linux</li>
<li>Fixed bug where Skinny session could go undetected</li>
<li>Fixed bug where NT service sometimes crashed when stopped by the user</li>
</ul>


<p>2005-11-02 Oreka 0.2</p>
<ul>
<li>VoIp plugin now supports Cisco skinny (aka SCCP) protocol (including metadata extraction)</li>
<li>VoIp plugin now supports Raw RTP sessions recording.</li>
<li>VoIP plugin now tries to associate a SIP call to a capture port that is made of the actual local SIP agent (phone) IP address and TCP port. Before that, a capture port could be made of a gateway or external IP address and TCP port.</li>
<li>Default to the last OS returned network device when config file does not specify or specifies invalid network device.</li>
<li>Visual studio project now opens without errors.</li>
<li>Fixed win32 crash when network device identifier in config file was invalid.</li>
</ul>
<p>2005-10-20 Oreka 0.1</p>
<ul>
<li>This is the first release of this software.</li>
</ul>
<?php
  include "../include/footer.inc.php";
?>
