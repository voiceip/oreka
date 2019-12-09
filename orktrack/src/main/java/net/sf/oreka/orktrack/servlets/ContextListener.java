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
package net.sf.oreka.orktrack.servlets;

import net.sf.oreka.orktrack.Constants;
import net.sf.oreka.orktrack.LogManager;
import net.sf.oreka.orktrack.OrkTrack;
import net.sf.oreka.util.TomcatServerXMLParser;
import org.apache.logging.log4j.Logger;

import javax.servlet.ServletContext;
import javax.servlet.ServletContextEvent;
import javax.servlet.ServletContextListener;


public class ContextListener implements ServletContextListener {

    Logger log = LogManager.getInstance().getRootLogger();

    public void contextDestroyed(ServletContextEvent arg0) {
        log.info(Constants.APP_NAME + " shutting down.");
    }

    public void contextInitialized(ServletContextEvent servletContextEvent) {

        ServletContext context = servletContextEvent.getServletContext();

        String configFolder = context.getInitParameter("ConfigDirectory");
        if (configFolder == null) {
            configFolder = "c:/oreka/";
        }

        String log4jConfigFile = context.getInitParameter("Log4jConfigFile");
        if (log4jConfigFile == null) {
            log.error("OrkTrack ContextInitialized() Log4jConfigFile context-param missing in web.xml");
        } else {
            log.info("OrkTrack ContextInitialized() log4jConfigFile is " + log4jConfigFile);
            log4jConfigFile = configFolder + "/" + log4jConfigFile;
        }

        String configFile = context.getInitParameter("ConfigFile");
        if (configFile == null) {
            log.error("OrkTrack ContextInitialized() ConfigFile context-param missing in web.xml");
        } else {
            log.info("OrkTrack ContextInitialized() configFile is " + configFile);
            configFile = configFolder + "/" + configFile;
        }

        String hibernateConfigFile = context.getInitParameter("HibernateConfigFile");
        if (hibernateConfigFile == null) {
            log.error("OrkTrack ContextInitialized() HibernateConfigFile context-param missing in web.xml");
        } else {
            log.info("OrkTrack ContextInitialized() HibernateConfigFile is " + hibernateConfigFile);
            hibernateConfigFile = configFolder + "/" + hibernateConfigFile;
        }

        // Get path to server.xml file
        if (log.isDebugEnabled())
            log.debug("OrkTrack ContextInitialized(): get Tomcat Home...");

        String tomcatHome = context.getInitParameter("TomcatHome");

        if (tomcatHome == null) {
            log.warn("OrkTrack ContextInitialized(): TomcatHome context-param missing in web.xml");
        } else {
            TomcatServerXMLParser.setTomcatHome(tomcatHome);
            log.info("OrkTrack ContextInitialized(): TomcatHome is set to " + tomcatHome);

            // Parse Tomcat's server.xml file and set the audio and screen paths among other things
            try {
                TomcatServerXMLParser.parseServerXML(tomcatHome);
            } catch (Exception e) {
                log.error("OrkTrack ContextInitialized() error parsing server.xml at " + tomcatHome);
            }
        }

        try{
            //PortManager.instance().initialize();
            OrkTrack.initialize(log4jConfigFile, hibernateConfigFile, configFile);
        } catch (Throwable e){
            log.error("OrkTrack ContextInitialized() threw exception",e);
        }
    }


}
