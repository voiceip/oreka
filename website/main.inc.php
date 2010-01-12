<?php
/*
 * Copyright 2004 Matt Brubeck
 * This file is licensed under a Creative Commons license:
 * http://creativecommons.org/licenses/by/2.0/
 */

// Work around PHP bug on sf.net where include_path gets reset.
set_include_path(".".PATH_SEPARATOR."/usr/share/pear".PATH_SEPARATOR."/usr/share/php");
require_once "include/lang.inc.php";
localization_setup();

$sitePath = ".";
$sectionId = "none";

$siteNavItems = array(
  array(_("Home"), "", 0),
  array(_("About"), "about/", 
    array(
      array(_("Features"), "features"),
      /*array(_("Screenshots"), "screenshots"),*/
      array(_("News"), "news"),
      array(_("License"), "license"),
      array(_("Credits"), "credits"),
      array(_("Links"), "links"),
    )
  ),
  array(_("Download"), "download/",
    array(
      array(_("OrkAudio Windows"), "windows"),
      array(_("OrkAudio Redhat/CentOS/Fedora"), "redhat"),
      array(_("OrkAudio Debian/Ubuntu"), "debian"),
      array(_("OrkAudio Gentoo"), "gentoo"),
      array(_("OrkWeb/OrkTrack"), "war"),
      array(_("Source Code"), "source"),
      array(_("Release Notes"), "release-notes"),
    )
  ),
  array(_("Support"), "support/",
    array(
      array(_("FAQ"), "faq"),
      array(_("Documentation"), "documentation"),
      array(_("Mailing Lists"), "lists"),
    )
  )
);
?>
