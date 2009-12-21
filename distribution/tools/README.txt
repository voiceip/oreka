Oreka 1.2 Open Source - Database Upgrade Script File

This updateOrekaDB_to_v1.sql script updates the Oreka database's old table and column names. It has been tested in MySQL only. 

Starting with Oreka version 570, some database table names and columns were modified.  This script can be run to upgrade the database prior to 
upgrading orkweb and orktrack on your system.  For details, read the header part of the script file itself.

Usage example, assuming an older version of Oreka was previoulsy running:

	mysql -uroot -p<password> <database_name> < updateOrekaDB_to_v1.sql      

	where the default <database_name> is oreka in Linux and test in Windows.