<?php
/*
 * This file contains code to set up and use the gettext-based localization
 * system.
 *
 * Copyright 2005 Matt Brubeck
 * This file is licensed under a Creative Commons license:
 * http://creativecommons.org/licenses/by/2.0/
 */

require_once 'HTTP.php';

// Locale configuration variables.
$domain = "audacity_website";
$lang = "en";
$encoding = "UTF-8";
$cookie_days = 90;

// List of available languages.
$available_locales = array(
  // Language code => Full locale name, Human-readable name.
  "cs" => array("cs_CZ", "Česky"),
  "de" => array("de_DE", "Deutsch"),
  "el" => array("el_GR", "Ελληνικά"),
  "en" => array("en_US", "English"),
  "es" => array("es_ES", "Espa&ntilde;ol"),
  "fr" => array("fr_FR", "Français"),
  "it" => array("it_IT", "Italiano"),
  "hu" => array("hu_HU", "Magyar"),
  "nb" => array("nb_NO", "Norsk (Bokm&aring;l)"),
  "pt" => array("pt_BR", "Portugu&ecirc;s"),
  "sl" => array("sl_SI", "Sloven&#353;&#269;ina"),
  "fi" => array("fi_FI", "Suomi"),
  "sv" => array("sv_SE", "Svenska"),
  "ja" => array("ja_JP", "日本語"),
  "ru" => array("ru_RU", "Русский"),
  "zh-CN" => array("zh_CN.UTF-8", "中文(简)"),
  "zh-TW" => array("zh_TW.UTF-8", "中文(繁)"),
);

// Set up the translation variables and libraries.
function localization_setup() {
  global $lang, $domain, $encoding, $available_locales, $preferred_lang;

  // Choose a default language based on the client's HTTP headers.
  // TODO: Replace HTTP::negotiateLanguage with something less buggy.
  // (See http://www.dracos.co.uk/web/php/HTTP/ for details.)
  $supported = $available_locales;
  $preferred_lang = HTTP::negotiateLanguage($supported, $lang);
  if ($preferred_lang)
    $lang = $preferred_lang;

  // Override the default if the user has an explicit cookie or query string.
  $force_lang = get_requested_lang();
  if ($force_lang && array_key_exists($force_lang, $available_locales))
    $lang = $force_lang;

  if ($available_locales[$lang]) {
    // Set the locale.
    $locale = $available_locales[$lang][0];
    setlocale(LC_ALL,  $locale);

    // Find the locale directory.
    $path_parts = pathinfo(__FILE__);
    $this_dir = $path_parts["dirname"];
    bindtextdomain($domain, "$this_dir/../locale");

    // Set up gettext message localization.
    textdomain($domain);
    bind_textdomain_codeset($domain, $encoding);
  }

  // Tell clients to cache different languages separately.
  header("Vary: Accept-Language");
}

// Return the explicitly requested language, if available.
function get_requested_lang() {
  global $cookie_days;
  $result = $_COOKIE["lang"];
  if ($_GET["lang"]) {
    $result = $_GET["lang"];
    setcookie("lang", $result, time() + $cookie_days*24*60*60, "/");
  }
  return $result;
}

// Convert a string from the locale encoding to the output encoding.
function locale_to_unicode($s) {
  global $encoding;
  $locale_encoding = nl_langinfo(CODESET);
  $s = iconv($locale_encoding, $encoding, $s);
  return $s;
}
?>
