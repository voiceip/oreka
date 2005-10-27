<?php
/*
 * Copyright 2004 Matt Brubeck
 * This file is licensed under a Creative Commons license:
 * http://creativecommons.org/licenses/by/2.0/
 */
  require_once "main.inc.php";
  require_once "../include/news.inc.php";
  $pageId = "news";

  $itemId = $_REQUEST["id"];

  if ($itemId != "") {
    $news_item = $news_items[$itemId];
    $pageTitle = $news_item->title;
    include "../include/header.inc.php";

    $dateStr = $news_item->dateStr();
    echo "<h2>$dateStr: $pageTitle</h2>";
    echo $news_item->body;
    echo '<hr><p><a href="news">'._("More news items...").'</a></p>';
  }
  else {
    // List news items in reverse chronological order.
    $pageTitle = _("News");
    include "../include/header.inc.php";

    echo "<h2>$pageTitle</h2>";
    echo "<ul>";
    foreach ($news_items as $key => $news_item) {
      echo '<li>'.$news_item->dateStr().": <a href=\"news?id=$key\">".$news_item->title.'</a></li>';
    }
    echo "</ul>";
  }

  include "../include/footer.inc.php";
?>
