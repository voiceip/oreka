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

import net.sf.oreka.OrkException;
import org.apache.log4j.BasicConfigurator;
import org.apache.logging.log4j.Logger;
import org.apache.logging.log4j.core.LoggerContext;
import org.apache.logging.log4j.core.config.ConfigurationSource;
import org.apache.logging.log4j.core.config.xml.XmlConfiguration;

import java.io.File;

/**
 * This singleton class manages all application log4j loggers
 */
public class LogManager {

	private static LogManager logManager = null;

	private Logger rootLogger = null;

	private LogManager()
	{
		rootLogger = org.apache.logging.log4j.LogManager.getRootLogger();
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
			//PropertyConfigurator.configure(ConfigFilename);
			LoggerContext context = (LoggerContext) org.apache.logging.log4j.LogManager.getContext(false);
			XmlConfiguration config = new XmlConfiguration(context, ConfigurationSource.fromUri(file.toURI()));
			context.start(config);
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