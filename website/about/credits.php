<?php
/*
 * Copyright 2003 Dominic Mazzoni
 * Copyright 2004 Matt Brubeck
 * This file is licensed under a Creative Commons license:
 * http://creativecommons.org/licenses/by/2.0/
 */
  require_once "main.inc.php";
  $pageId = "credits";
  $pageTitle = _("Credits");
  include "../include/header.inc.php";
?>

<h2><?=$pageTitle?></h2>
<h3><?=_("Libraries")?></h3>
<p><?=_("Oreka uses open source libraries from the following projects, many thanks to their authors and contributors:")?></p>
<p>C++ libraries</p>
<ul>
<li><a href="http://www.mega-nerd.com/libsndfile/">libsndfile</a></li>
<li><a href="http://www.portaudio.com/">PortAudio</a></li>
<li><a href="http://logging.apache.org/log4cxx/">log4cxx</a></li>
<li><a href="http://www.cs.wustl.edu/~schmidt/ACE.html">ACE</a></li>
<li><a href="http://www.tcpdump.org/">libpcap</a></li>
<li><a href="http://www.boost.org">boost</a></li>
<li><a href="http://xml.apache.org/xerces-c/">xerces</a></li>
</ul>

<p>Java libraries</p>
<ul>
<li><a href="http://jakarta.apache.org/tapestry/">Tapestry</a></li>
<li><a href="http://www.hibernate.org">Hibernate</li>
<li><a href="http://logging.apache.org/log4j/docs/index.html">log4j</a></li>
<li>And various other libraries used by those libraries.</li>
</ul>
<?php
  include "../include/footer.inc.php";
?>
