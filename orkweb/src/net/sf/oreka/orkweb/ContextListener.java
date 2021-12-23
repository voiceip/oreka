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

import net.sf.oreka.util.TomcatServerXMLParser;

import org.apache.logging.log4j.Logger;

public class ContextListener implements ServletContextListener {

	Logger log = org.apache.logging.log4j.LogManager.getRootLogger();
	public static boolean debugSwitch = false; // Enables application debugging (e.g. no login phase)

	public void contextInitialized(ServletContextEvent servletContextEvent) {
		
		ServletContext context = servletContextEvent.getServletContext();

		String configFolder = context.getInitParameter("ConfigDirectory");
		if (configFolder == null) {
			configFolder = "c:/oreka/";
		}

		String log4jConfigFile = context.getInitParameter("Log4jConfigFile");
		if (log4jConfigFile == null) {
			log.error("OrkWeb ContextInitialized(): Log4jConfigFile context-param missing in web.xml");
		} else {
			log4jConfigFile = configFolder + "/" + log4jConfigFile;
			try {
				LogManager.getInstance().configure(log4jConfigFile);
				log.info("OrkWeb ContextInitialized(): log4jConfigFile is " + log4jConfigFile);
			} catch (Throwable e){
				e.printStackTrace();
				log.error("OrkWeb ContextInitialized(): Error configuring log4j: " + e.getMessage());
			}

		}

		log.info("========================================");
		log.info("OrkWeb starting ...");

		String hibernateConfigFile = context.getInitParameter("HibernateConfigFile");
		if (hibernateConfigFile == null) {
			log.error("OrkWeb ContextInitialized(): HibernateConfigFile context-param missing in web.xml");
		} else {
			log.info("OrkWeb ContextInitialized(): HibernateConfigFile is " + hibernateConfigFile);
			hibernateConfigFile  = configFolder + "/" + hibernateConfigFile ; 
		}
				
		try {
			OrkWeb.hibernateManager.configure(hibernateConfigFile);
		}
		catch (Throwable e) {
			e.printStackTrace();
			log.error("OrkWeb ContextInitialized(): Error configuring Hibernate: " + e.getMessage());
		}
		
		// Get path to server.xml file
		if (log.isDebugEnabled())
			log.debug("OrkWeb ContextInitialized(): get Tomcat Home...");

		String tomcatHome = context.getInitParameter("TomcatHome");

		if (tomcatHome == null) {
			log.warn("OrkWeb ContextInitialized(): TomcatHome context-param missing in web.xml");
		} else {
			TomcatServerXMLParser.setTomcatHome(tomcatHome);
			log.info("OrkWeb ContextInitialized(): TomcatHome is set to " + tomcatHome);

			// Parse Tomcat's server.xml file and set the audio and screen paths among other things
			try {
				TomcatServerXMLParser.parseServerXML(tomcatHome);
			} catch (Exception e) {
				log.error("OrkWeb ContextInitialized() error parsing server.xml at " + tomcatHome);
			}			
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
		
		log.info("OrkWeb started successfully.");
		log.info("----------------------------------------");

	}
	
	public void contextDestroyed(ServletContextEvent arg0) {
		log.info("orkweb stopping ...");
		log.info("========================================");
	}	
}