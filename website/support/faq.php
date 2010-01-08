<?php
/*
 * Copyright 2003 Dominic Mazzoni
 * Copyright 2005 Matt Brubeck
 * This file is licensed under a Creative Commons license:
 * http://creativecommons.org/licenses/by/2.0/
 */
  require_once "main.inc.php";
  $pageId = "faq";

  if ($_REQUEST["section"])
    $faqSectionId = $_REQUEST["section"];
  else
    $faqSectionId = $_REQUEST["s"];

  if ($_REQUEST["item"])
    $itemId = $_REQUEST["item"];
  else
    $itemId = $_REQUEST["i"];

  $faqSections = array(
    "general" => array(
      _("About Oreka"),
      array(
        "filenaming" => array(
          _("Can I change the path and filename of the audio recordings?"),
          _("<p>There are configuration options in the config.xml for orkaudio specifically for that.  See <a href='http://oreka.sourceforge.net/oreka-user-manual.html#filenaming'>Configure audio files path and file names</a> section in the Oreka User Manual.</p>")
        ),
        "G729" => array(
          _("Why can't I record G.729 voice packets?"),
          _("<p>G.729 requires licensing, therefore it is not supported by Oreka Open Source</p>")
        ),
        "voiptraffic" => array(
          _("Any recommendations on how to get VoIP traffic to the Oreka server?"),
          _("<p>See <a href='http://oreka.sourceforge.net/oreka-user-manual.html#gettingvoiptraffic'>Getting VoIP traffic to the Oreka Server</a> section in the Oreka User Manual.</p>")
        ),
        "difference" => array(
          _("What is the difference between this software and other open source audio recording software?"),
          _("<p>Oreka is specifically designed for recording of lots of audio sessions such as telephone calls and easy retrieval through a web interface. Existing software such as Audacity or Ardour focus on different things such as audio edition and studio quality recording respectively.</p>")
        ),
       "threedifferent" => array(
          _("Why is the system broken up in three services ? Would it not have been easier to implement everything in one service?"),
          _("<p>This allows for greater flexibility and scalability. It means for example that it's possible to gather and log data from multiple OrkAudio recorders to one central database.</p>")
        ),
       "separately" => array(
          _("Is it possible to use the different components (OrkAudio, OrkTrack and OrkWeb) separately?"),
          _("<p>Yes. For example it is possible to use OrkAudio as a standalone audio recording and compression tool. No user interface is available but audio files and metadata are generated as described in Oreka <a href='documentation'>documentation</a>.</p>")
        ),
       "H.323" => array(
          _("My telephony system uses H.323 as the signalling protocol. Will Oreka be able to record audio from it ?"),
          _("<p>Yes. Systems using H.323 as the signalling protocol use RTP as the media transport protocol. Oreka will capture raw RTP audio sessions with no metadata attached and IP addresses as local and remote parties.</p>")
        ),
     ),
    ),
  );

  if ($faqSectionId != "" && $itemId != "") {
    // Print the requested item.
    $item = $faqSections[$faqSectionId][1][$itemId];

    $question = $item[0];
    $answer   = $item[1];

    $pageTitle = $question;
    include "../include/header.inc.php";

    echo "<h2>$question</h2>";
    echo $answer;

    echo "<h3><a href=\"faq\">"._("Other frequently asked questions...")."</a></h3>";
  }
  else {
    // Print the list of sections and questions.
    $pageTitle = _("Frequently Asked Questions");
    include "../include/header.inc.php";

    echo "<h2>$pageTitle</h2>";
    echo "<p>"._('These are frequently asked questions about Oreka. If you cannot find what you are looking for here, see the <a href="documentation">documentation</a>, or ask on the Oreka mailing lists')."</p>";

    foreach ($faqSections as $faqSectionId => $section) {
      $sectionTitle = $section[0];
      $sectionItems = $section[1];

      echo "<h3 id=\"s$faqSectionId\">$sectionTitle</h3>";

      echo "<ol>";
      foreach ($sectionItems as $itemId => $item) {
        $question = $item[0];
        $answer   = $item[1];

        echo "<li><a href=\"faq?s=$faqSectionId&amp;i=$itemId\">$question</a></li>";
      }
      echo "</ol>";
    }
  }

  include "../include/footer.inc.php";
?>
