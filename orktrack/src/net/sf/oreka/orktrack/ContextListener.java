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

import javax.servlet.ServletContext;
import javax.servlet.ServletContextEvent;
import javax.servlet.ServletContextListener;

import org.apache.log4j.Logger;

public class ContextListener implements ServletContextListener {

	Logger log = LogManager.getInstance().getContextLogger();
	
	public void contextDestroyed(ServletContextEvent arg0) {

		log.info(OrkTrack.APP_NAME + " shutting down.");
	}

	public void contextInitialized(ServletContextEvent servletContextEvent) {
		
		ServletContext context = servletContextEvent.getServletContext();
		
		String configFolder = context.getInitParameter("ConfigDirectory");
		if (configFolder == null) {
			configFolder = "c:/oreka/";
		}

		String log4jConfigFile = context.getInitParameter("Log4jConfigFile");	
		if (log4jConfigFile == null) {
			log.error("Log4jConfigFile context-param missing in web.xml");
		} else {
			log.info("log4jConfigFile is " + log4jConfigFile);
			log4jConfigFile = configFolder + "/" + log4jConfigFile; 
		}

		String configFile = context.getInitParameter("ConfigFile");
		if (configFile == null) {
			log.error("ConfigFile context-param missing in web.xml");
		} else {
			log.info("configFile is " + configFile);
			configFile = configFolder + "/" + configFile; 
		}

		String hibernateConfigFile = context.getInitParameter("HibernateConfigFile");
		if (hibernateConfigFile == null) {
			log.error("HibernateConfigFile context-param missing in web.xml");
		} else {
			log.info("HibernateConfigFile is " + hibernateConfigFile);
			hibernateConfigFile  = configFolder + "/" + hibernateConfigFile ; 
		}
		
		//PortManager.instance().initialize();
		
		OrkTrack.initialize(log4jConfigFile, hibernateConfigFile, configFile);
	}

	
}
