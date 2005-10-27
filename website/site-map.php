<?php
/*
 * Copyright 2004 Matt Brubeck
 * This file is licensed under a Creative Commons license:
 * http://creativecommons.org/licenses/by/2.0/
 */
  require_once "main.inc.php";
  $pageTitle = _("Site Map");
  include "include/header.inc.php";
?>

<h2><?=$pageTitle?></h2>
<ul>
<?php
  foreach ($siteNavItems as $navItem) {
    $name = $navItem[0];
    $sectionPath = $navItem[1];
    $sectionNavItems = $navItem[2];
    echo "<li><a href=\"$sitePath/$sectionPath\">$name</a>";
    if ($sectionNavItems) {
      echo "<ul>";
      foreach ($sectionNavItems as $navItem) {
        $name = $navItem[0];
        $path = $navItem[1];
        echo "<li><a href=\"$sitePath/$sectionPath$path\">$name</a></li>";
      }
      echo "</ul>";
    }
		echo "</li>";
  }
?>
</ul>
<?php
  include "include/footer.inc.php";
?>
