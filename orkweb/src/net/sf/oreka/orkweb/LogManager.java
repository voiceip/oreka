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

package net.sf.oreka.orkweb;

import java.io.File;

import org.apache.log4j.BasicConfigurator;
import org.apache.log4j.Logger;
import org.apache.log4j.PropertyConfigurator;

/**
 * This singleton class manages all application log4j loggers
 */
public class LogManager {

	static LogManager logManager = null;
	
	Logger rootLogger = null;

	
	private LogManager() 
	{
		rootLogger = Logger.getRootLogger();
		
	    BasicConfigurator.configure();	// in case there is no properties file
	}
	
	public static LogManager getInstance()
	{
		if (logManager == null)
		{
			logManager = new LogManager();
		}
		return logManager;
	}

	public void configure(String filename) {
		
		// Check wether filename is valid
		File file = new File(filename);
		if (file.exists()) {
			// Attempt to configure log4j
			PropertyConfigurator.configure(filename);
		}
		else {
			rootLogger.warn("Log4j properties file does not exist:" + filename + " check your web.xml");
		}
	}
	
	/**
	 * @return Returns the rootLogger.
	 */
	public Logger getRootLogger() {
		return rootLogger;
	}
	

	/**
	 * @param rootLogger The rootLogger to set.
	 */
	public void setRootLogger(Logger rootLogger) {
		this.rootLogger = rootLogger;
	}
}
