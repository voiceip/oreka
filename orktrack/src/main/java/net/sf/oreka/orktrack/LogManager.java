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

import net.sf.oreka.OrkException;
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

	private String ConfigFilename = null;

	private Logger rootLogger = null;
	private Logger configLogger = null;
	private Logger contextLogger = null;
	private Logger portLogger = null;
	private Logger userLogger = null;
	private Logger recurrentLogger = null;	// special logger for recurrent messages (annoying to have normally)
	
	private LogManager()
	{
		rootLogger = org.apache.logging.log4j.LogManager.getRootLogger();
 		configLogger = org.apache.logging.log4j.LogManager.getLogger("config");
		contextLogger = org.apache.logging.log4j.LogManager.getLogger("context");
		portLogger = org.apache.logging.log4j.LogManager.getLogger("port");
		userLogger = org.apache.logging.log4j.LogManager.getLogger("user");
		recurrentLogger = org.apache.logging.log4j.LogManager.getLogger("net.sf.oreka.recurrent");
		
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
			//PropertyConfigurator.configure(ConfigFilename);
            LoggerContext context = (org.apache.logging.log4j.core.LoggerContext) org.apache.logging.log4j.LogManager.getContext(false);
            XmlConfiguration config = new XmlConfiguration(context, ConfigurationSource.fromUri(file.toURI()));
            context.start(config);

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