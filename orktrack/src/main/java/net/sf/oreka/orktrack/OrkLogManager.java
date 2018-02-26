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

package net.sf.oreka.orktrack;

import java.io.File;

import net.sf.oreka.OrkException;

import org.apache.log4j.BasicConfigurator;
import org.apache.log4j.Level;
import org.apache.log4j.Logger;
import org.apache.log4j.PropertyConfigurator;

/**
 * This singleton class manages all application log4j loggers
 */
public class OrkLogManager {

	private static OrkLogManager logManager = null;

	private String ConfigFilename = null;

	private org.apache.log4j.Logger rootLogger = null;
	private org.apache.log4j.Logger configLogger = null;
	private org.apache.log4j.Logger contextLogger = null;
	private org.apache.log4j.Logger portLogger = null;
	private org.apache.log4j.Logger userLogger = null;
	private org.apache.log4j.Logger recurrentLogger = null;	// special logger for recurrent messages (annoying to have normally)
	
	private OrkLogManager()
	{
		rootLogger = org.apache.log4j.Logger.getRootLogger();
		rootLogger.setLevel(Level.INFO);
		configLogger = org.apache.log4j.Logger.getLogger("config");
		contextLogger = org.apache.log4j.Logger.getLogger("context");
		portLogger = org.apache.log4j.Logger.getLogger("port");
		userLogger = org.apache.log4j.Logger.getLogger("user");
		recurrentLogger = org.apache.log4j.Logger.getLogger("net.sf.oreka.recurrent");
		
	    BasicConfigurator.configure();	// in case there is no properties file
	}
	
	public static OrkLogManager getInstance()
	{
		if (logManager == null)
		{
			logManager = new OrkLogManager();
		}
		return logManager;
	}

	public void configure(String filename) throws OrkException {
		
		ConfigFilename = filename;
		configure();
	}
	
	public void configure() throws OrkException {
		
		// Check wether filename is valid
		File file = new File(ConfigFilename);
		if (file.exists()) {

			// Attempt to configure log4j
			PropertyConfigurator.configure(ConfigFilename);
		}
		else {
			throw new OrkException("Log4j properties file does not exist:" + ConfigFilename + " check your web.xml");
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

	/**
	 * @return Returns the configLogger.
	 */
	public Logger getConfigLogger() {
		return configLogger;
	}
	

	/**
	 * @param configLogger The configLogger to set.
	 */
	public void setConfigLogger(Logger configLogger) {
		this.configLogger = configLogger;
	}

	public Logger getContextLogger() {
		return contextLogger;
	}

	public void setContextLogger(Logger contextLogger) {
		this.contextLogger = contextLogger;
	}

	public Logger getPortLogger() {
		return portLogger;
	}

	public void setPortLogger(Logger portLogger) {
		this.portLogger = portLogger;
	}

	public Logger getUserLogger() {
		return userLogger;
	}

	public void setUserLogger(Logger userLogger) {
		this.userLogger = userLogger;
	}

	public Logger getRecurrentLogger() {
		return recurrentLogger;
	}
	

	public void setRecurrentLogger(Logger recurrentLogger) {
		this.recurrentLogger = recurrentLogger;
	}
	
	
	

}