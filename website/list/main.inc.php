<?php

$charset =             "iso-8859-1";

function Top($title)
{
	header("Content-type: text/html; charset=$charset");

	print "<html><head><title>$title</title></head>\n";

	print "<body bgcolor=\"#ffffff\">\n";

	print "<center>\n";
	print "<a href=\"http://oreka.sourceforge.net\">\n";
	print "<img src=../images/oreka.jpg width=253 height=100 border=0>\n";
	print "</a>\n";

	print '<table border="0" cellpadding="1" cellspacing="0" bgcolor="#000099" width="80%"><tr><td>';
	print '<table border="0" cellspacing="1" cellpadding="4" width="100%">';
	print '<tr><td align="center" bgcolor="#dddddd">';
	print '<font size="+1"><b>';
	print $title;
	print '<b></font>';
	print '</td></tr>';
	print '<tr><td align="left" bgcolor="#ffffff">';
}

function Bottom()
{
	print '</td></tr></table></td></tr></table></center>';
	print '</body></html>';
}

?>
