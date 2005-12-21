/*
 * Oreka -- A media capture and retrieval platform
 * 
 * Copyright (C) 2005, orecx LLC
 *
 * http://www.orecx.com
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License.
 * Please refer to http://www.gnu.org/copyleft/gpl.html
 *
 */

/**
 * 
 */
package net.sf.oreka.orktrack;

import java.util.Date;

import net.sf.oreka.HibernateManager;
import net.sf.oreka.OrkObjectFactory;
import net.sf.oreka.orktrack.messages.ConfigureLogMessage;
import net.sf.oreka.orktrack.messages.MetadataMessage;
import net.sf.oreka.orktrack.messages.PingMessage;
import net.sf.oreka.orktrack.messages.TapeMessage;
import net.sf.oreka.orktrack.messages.UserStateMessage;

import org.apache.log4j.Level;
import org.apache.log4j.Logger;



public class OrkTrack {

	public static final String APP_NAME = "OrkTrack";
	
	public static HibernateManager hibernateManager = HibernateManager.instance();
	private static Date lastInMemoryObjectsSync = new Date(0);
	
	static Logger logger = Logger.getLogger(OrkTrack.class);
	
	public OrkTrack() {
		
		LogManager.getInstance().getConfigLogger().log(Level.INFO, "Entering OrkTrack");
	}
	
	public static void initialize(String log4jConfigFile, String hibernateConfigFile, String configFile) {
		
		try {
			LogManager.getInstance().configure(log4jConfigFile);

			logger.info("========================================");
			logger.info(OrkTrack.APP_NAME + " starting ...");
			
			// Register all OrkObjects
			OrkObjectFactory.instance().registerOrkObject(new OrkTrackConfig());	
			OrkObjectFactory.instance().registerOrkObject(new MetadataMessage());
			OrkObjectFactory.instance().registerOrkObject(new TapeMessage());
			OrkObjectFactory.instance().registerOrkObject(new UserStateMessage());
			OrkObjectFactory.instance().registerOrkObject(new PingMessage());
			OrkObjectFactory.instance().registerOrkObject(new ConfigureLogMessage());
			ConfigManager.getInstance().load(configFile);

			hibernateManager.configure(hibernateConfigFile);
		}
		catch (Exception e) {
			logger.error("OrkTrack.initialize: Error configuring Hibernate:" + e.getMessage());				
		}
		
		/*
		boolean initOk = false;
		//while(initOk == false) {
			initOk  = PortManager.instance().initialize();
			if (initOk) {
				initOk = ProgramManager.instance().load();
			}
			//try{Thread.sleep(5000);} catch (Exception e) {};
		//}
		 */
		refreshInMemoryObjects();
	}
	
	public static void refreshInMemoryObjects() {
		Date now = new Date();
		if((now.getTime() - lastInMemoryObjectsSync.getTime()) > 5000) {
			LogManager.getInstance().getRecurrentLogger().debug("Refreshing In-Memory objects");
			// refresh every 5 seconds
			lastInMemoryObjectsSync = now;
			PortManager.instance().initialize();
			ProgramManager.instance().load();
		}
	}
	
	public static void main(String[] args)
	{
		//System.out.println("hello");
		//RecSegment seg = new RecSegment();
		//System.out.println(seg.getDuration());
		OrkTrack orkTrack = new OrkTrack();
	}

}
