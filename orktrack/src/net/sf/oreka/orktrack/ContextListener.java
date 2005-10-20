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
		String log4jConfigFile = context.getInitParameter("Log4jConfigFile");
		
//		boolean complainAboutLog4jConfigFile = false;
//		if (log4jConfigFile != null) {
//			LogManager.getInstance().configure(log4jConfigFile);
//		}
//		else {
//			complainAboutLog4jConfigFile = true;
//		}
		if (log4jConfigFile == null) {
			System.out.println("Log4jConfigFile context-param missing in web.xml");
		}
		
//		log.info("========================================");
//		log.info(OrkTrack.APP_NAME + " starting ...");
		
//		if(complainAboutLog4jConfigFile) {
//			log.warn("contextInitialized: Log4jConfigFile context-param missing in web.xml");
//		}
		
		// Register all OrkObjects
//		OrkObjectFactory.instance().registerOrkObject(new OrkTrackConfig());	
		
		String configFile = context.getInitParameter("ConfigFile");
//		if (configFile != null) {
//			ConfigManager.getInstance().load(configFile);
//		}
//		else {
//			log.error("contextInitialized: ConfigFile context-param missing in web.xml");			
//		}
		if (configFile == null) {
			System.out.println("ConfigFile context-param missing in web.xml");
		}
		
		String hibernateConfigFile = context.getInitParameter("HibernateConfigFile");
//		if (hibernateConfigFile != null) {
//			try {
//				HibernateManager.configure(hibernateConfigFile);
//			}
//			catch (Exception e) {
//				log.error("contextInitialized: Error configuring Hibernate:" + e.getMessage());				
//			}
//		}
//		else {
//			log.error("contextInitialized: HibernateConfigFile context-param missing in web.xml");			
//		}
		if (hibernateConfigFile == null) {
			System.out.println("HibernateConfigFile context-param missing in web.xml");
		}
		
		//PortManager.instance().initialize();
		
		OrkTrack.initialize(log4jConfigFile, hibernateConfigFile, configFile);
	}

	
}
