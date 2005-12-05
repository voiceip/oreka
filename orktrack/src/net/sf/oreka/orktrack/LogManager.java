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
public class LogManager {

	static LogManager logManager = null;
	
	String ConfigFilename = null;
	
	Logger rootLogger = null;
	Logger configLogger = null;
	Logger contextLogger = null;
	Logger portLogger = null;
	Logger userLogger = null;
	Logger recurrentLogger = null;	// special logger for recurrent messages (annoying to have normally)
	
	private LogManager() 
	{
		rootLogger = Logger.getRootLogger();
		rootLogger.setLevel(Level.INFO);
		configLogger = Logger.getLogger("config");
		contextLogger = Logger.getLogger("context");
		portLogger = Logger.getLogger("port");
		userLogger = Logger.getLogger("user");
		recurrentLogger = Logger.getLogger("net.sf.oreka.recurrent");
		
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