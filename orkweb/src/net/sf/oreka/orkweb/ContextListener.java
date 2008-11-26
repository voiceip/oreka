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

import javax.servlet.ServletContext;
import javax.servlet.ServletContextEvent;
import javax.servlet.ServletContextListener;

import org.apache.log4j.Logger;

public class ContextListener implements ServletContextListener {
	
	Logger log = null;
	public static boolean debugSwitch = false; // Enables application debugging (e.g. no login phase)
	
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
			log4jConfigFile = configFolder + "/" + log4jConfigFile;
			LogManager.getInstance().configure(log4jConfigFile);
			log = LogManager.getInstance().getRootLogger();
			log.info("log4jConfigFile is " + log4jConfigFile);
		}

		log.info("========================================");
		log.info("orkweb starting ...");

		String hibernateConfigFile = context.getInitParameter("HibernateConfigFile");
		if (hibernateConfigFile == null) {
			log.error("HibernateConfigFile context-param missing in web.xml");
		} else {
			log.info("HibernateConfigFile is " + hibernateConfigFile);
			hibernateConfigFile  = configFolder + "/" + hibernateConfigFile ; 
		}
				
		try {
			OrkWeb.hibernateManager.configure(hibernateConfigFile);
		}
		catch (Exception e) {
			log.error("orkweb.ContextListener: Error configuring Hibernate: " + e.getMessage());				
		}
		
		String debugSwitchParm = context.getInitParameter("Debug");
		if (debugSwitchParm != null) {
			if(debugSwitchParm.equals("true")) {
				debugSwitch = true;
				log.warn("Running in debug mode");				
			}
			else {
				debugSwitch = false;
			}
		}
	}
	
	public void contextDestroyed(ServletContextEvent arg0) {
		log.info("orkweb stopping ...");
		log.info("========================================");
	}	
}