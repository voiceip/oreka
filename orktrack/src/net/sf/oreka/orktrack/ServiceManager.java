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

import net.sf.oreka.persistent.Service;

import org.apache.log4j.Logger;
import org.hibernate.Session;

public class ServiceManager {
	
	static Logger logger = Logger.getLogger(ServiceManager.class);
	
	public static Service retrieveOrCreate(String name, String hostname, Session hbnSession) {
		Service service = retrieveByName(name, hbnSession);
		if (service == null) {
			logger.info("Creating service:" + name + " on host:" + hostname );
			service = new Service();
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
	
	public static Service retrieveByName(String name, Session hbnSession) {
		
		Service service = null;
		List services = hbnSession.createQuery(
	    "from Service as srv where srv.name=:name")
	    .setString("name", name)
	    .list();
		if (services.size() > 0) {
			service =  (Service)services.get(0);
		}
		return service;
	}
}
