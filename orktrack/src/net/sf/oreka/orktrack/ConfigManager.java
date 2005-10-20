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
package net.sf.oreka.orktrack;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.InputStream;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;

import net.sf.oreka.OrkException;
import net.sf.oreka.serializers.DomSerializer;

import org.apache.log4j.Logger;
import org.w3c.dom.Document;

/**
 * This singleton class manages all application configuration parameters
 */
public class ConfigManager {

	static ConfigManager configManager = null;
	
	Logger log = null;
	OrkTrackConfig config = null;
	
	private ConfigManager() {
		config = new OrkTrackConfig();
		log = LogManager.getInstance().getConfigLogger();
	}
	
	public static ConfigManager getInstance()
	{
		if (configManager == null)
		{
			configManager = new ConfigManager();
		}
		return configManager;
	}
	
	public void load(String filename) {
		
		DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
		try {
			File file = new File(filename);
			DocumentBuilder builder = factory.newDocumentBuilder();
			InputStream is = new FileInputStream(file);
			Document doc = builder.parse(is);
			DomSerializer ser = new DomSerializer();
			config = (OrkTrackConfig)ser.deSerialize(doc.getFirstChild());
		}
		catch (OrkException e) {
			log.error("ConfigManager.load: deserialization error:" + e.getMessage());
		}
		catch (FileNotFoundException e) {
			log.error("ConfigManager.load config file does not exist:" + filename + " check your web.xml");
		}
		catch (Exception e) {
			log.error("ConfigManager.load: exception:" + e.getMessage());
		}
	}

	public OrkTrackConfig getConfig() {
		return config;
	} 

	public void setConfig(OrkTrackConfig config) {
		this.config = config;
	}
}
