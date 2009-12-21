-- -----------------------------------------------------------------------------
-- Oreka 1.2 Open Source
-- Database upgrade script
--
-- Summary:
--      This script updates the Oreka database's old table and column names.
--      It has been tested in MySQL only. 
--
-- History:
--      Starting with Oreka version 570, some database table names and columns
--      were modified.  This script can be run to upgrade the database prior to 
--      upgrading orkweb and orktrack on your system.  Below are the details of
--      what gets changed.
--
-- Usage:
--      Run this script prior to upgrading orkweb and orktrack
-- 
--      Example: 
--         mysql -uroot -p<password> <database_name> < updateOrekaDB_to_v1.sql      
--
--      where the default <database_name> is oreka in Linux and test in Windows.
-- 
-- Details:
--      The following tables will be renamed:
--
--        domain        -> orkdomain
--        loginstring   -> orkloginString
--        recport       -> orkport
--        recportface   -> orkportface
--        recprogram    -> orkprogram
--        recsegment    -> orksegment
--        recsession    -> orksession
--        rectape       -> orktape
--        service       -> orkservice
--        user          -> orkuser
--
--      The following columns will also be renamed:
--        in orkportface
--             recport_id becomes port_id
--        in orkprogram
--             owner_id becomes creator_id
--        in orksegment
--             recSession_id becomes session_id
--             recSessionOffset becomes sessionOffset
--             recTape_id becomes tape_id
--             recTapeOffset becomes tapeOffset
--             recPort_id becomes port_id
--             recPortName becomes portName
--         in orktape
--             recPort_id becomes port_id
--             recPortName becomes portName
--
-- -----------------------------------------------------------------------------

RENAME TABLE domain TO orkdomain;
RENAME TABLE loginstring TO orkloginstring;
RENAME TABLE recport TO orkport;
RENAME TABLE recportface TO orkportface;
RENAME TABLE recprogram TO orkprogram;
RENAME TABLE recsegment TO orksegment;
RENAME TABLE recsession TO orksession;
RENAME TABLE rectape TO orktape;
RENAME TABLE service TO orkservice;
RENAME TABLE user TO orkuser;
RENAME TABLE progtoseg TO orkprogtoseg;    

ALTER TABLE orkportface CHANGE recPort_id port_id INT(11); 
ALTER TABLE orkprogram  CHANGE owner_id creator_id INT(11);
ALTER TABLE orksegment  CHANGE recSession_id session_id INT(11);   
ALTER TABLE orksegment  CHANGE recSessionOffset sessionOffest BIGINT(20) NOT NULL;
ALTER TABLE orksegment  CHANGE recTape_id tape_id INT(11);
ALTER TABLE orksegment  CHANGE recTapeOffset tapeOffset BIGINT(20) NOT NULL;
ALTER TABLE orksegment  CHANGE recPort_id port_id INT(11);
ALTER TABLE orksegment  CHANGE recPortName portName VARCHAR(255);
ALTER TABLE orktape     CHANGE recPort_id port_id INT(11);
ALTER TABLE orktape     CHANGE recPortName portName VARCHAR(255);
ALTER TABLE orkprogtoseg CHANGE SegId segId INT(11) NOT NULL;
ALTER TABLE orkprogtoseg CHANGE ProgId progId INT(11) NOT NULL;
