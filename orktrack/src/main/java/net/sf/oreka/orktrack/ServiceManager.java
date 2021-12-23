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

import java.util.List;

import net.sf.oreka.persistent.OrkService;

import org.apache.log4j.Logger;
import org.hibernate.Session;

public class ServiceManager {
	
	static Logger logger = Logger.getLogger(ServiceManager.class);
	
	public static OrkService retrieveOrCreate(String name, String hostname, Session hbnSession) {
		OrkService service = retrieveByName(name, hbnSession);
		if (service == null) {
			logger.info("Creating service:" + name + " on host:" + hostname );
			service = new OrkService();
			service.setName(name);
			service.setHostname(hostname);
			service.setFileServeProtocol("http");
			service.setFileServeTcpPort(8080);
			service.setRecordMaster(true);
			hbnSession.save(service);
		}
		else {
			//service.setHostname(hostname);	// update hostname in case it has changed
		}
		return service;
	}
	
	public static OrkService retrieveByName(String name, Session hbnSession) {
		
		OrkService service = null;
		List services = hbnSession.createQuery(
	    "from OrkService as srv where srv.name=:name")
	    .setString("name", name)
	    .list();
		if (services.size() > 0) {
			service =  (OrkService)services.get(0);
		}
		return service;
	}
}
