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

/**
 * 
 */
package net.sf.oreka.services;

import java.util.*;
import java.io.*;
import org.apache.log4j.Level;
import org.apache.log4j.Logger;
import org.apache.log4j.PropertyConfigurator;

public class Startup {

	static Logger logger = Logger.getLogger(Startup.class);	
	
	public Startup() {
		
		logger.log(Level.WARN, "Initializing orkweb");
		
		InputStream is = this.getClass().getClassLoader().getResourceAsStream("log4j.properties");
		if (is != null)
		{
			Properties properties = new Properties();
			try
			{
				properties.load(is);
				PropertyConfigurator.configure(properties);
			}
			catch (IOException e)
			{
				logger.log(Level.INFO, "log4j.properties not found");	
			}
		}
		
		//PropertyConfigurator.configure("log4j.properties");
		
		/*
		//InputStream s = ClassLoader.getSystemResourceAsStream("toto.txt");
		InputStream s = this.getClass().getClassLoader().getResourceAsStream("toto.txt");
		if (s != null)
		{
			logger.log(Level.INFO, "~~~~~~~~ yes");				
		}
		else
		{
			logger.log(Level.INFO, "~~~~~~~~ no");	
		}
		*/


	}

}
