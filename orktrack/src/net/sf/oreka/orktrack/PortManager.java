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

import java.util.HashMap;
import java.util.Iterator;
import java.util.List;

import net.sf.oreka.persistent.OrkPort;
import net.sf.oreka.persistent.OrkPortFace;
import net.sf.oreka.persistent.OrkService;

import org.apache.log4j.Logger;
import org.hibernate.Session;
import org.hibernate.Transaction;

/** Singleton class that manages activity on all capture ports */
public class PortManager {

	HashMap<Integer, Port> portById = new HashMap<Integer, Port>();
	HashMap<String, Port> portByName = new HashMap<String, Port>();
	
	static Logger logger = Logger.getLogger(PortManager.class);
	
	static PortManager portManager = null;
	
	private PortManager () {
	}
	
	public static PortManager instance() {
		if (portManager == null) {
			portManager = new PortManager();
		}
		return portManager;
	}
	
	public Port getPort(String name) {
		return portByName.get(name);
	}
	
	public Port getAndCreatePort(String name, Session hbnSession, OrkService service) {
		
		Port port = portByName.get(name);
		if (port == null) {
			
			port = createPort(name, hbnSession, service);
		}
		return port; 
	}
	
	public synchronized Port createPort(String name, Session hbnSession, OrkService service) {
		
		OrkPort recPort = null;

		OrkPortFace portFace = (OrkPortFace)hbnSession.get(OrkPortFace.class, name);
		if (portFace == null) {
			portFace  = new OrkPortFace();
			portFace.setName(name);
			portFace.setPort(recPort);
			portFace.setService(service);
			
			recPort = new OrkPort();
			portFace.setPort(recPort);
		}
		else {
			portFace.setPort(recPort);
			portFace.setService(service);
			recPort = portFace.getPort();
		}
		
		hbnSession.save(recPort);
		hbnSession.save(portFace);
		
		logger.info("created port:" + recPort.getId() + " with face:" + name);
		
    	Port port = new Port(recPort);
    	port.portFaces.add(portFace);
    	portById.put(recPort.getId(), port);
    	portByName.put(portFace.getName(), port);
		return port;
	}
	
	// for testing purposes
	public void addPort(String face1, String face2, Session hbnSession) {
		
		OrkPort recPort = new OrkPort();
		Port port = new Port(recPort);
		OrkPortFace portFace1 = new OrkPortFace();
		portFace1.setName(face1);
		portFace1.setPort(recPort);
		OrkPortFace portFace2 = new OrkPortFace();
		portFace2.setName(face2);
		portFace2.setPort(recPort);
		
		hbnSession.save(recPort);
		hbnSession.save(portFace1);
		hbnSession.save(portFace2);
		
		port.portFaces.add(portFace1);
		port.portFaces.add(portFace2);	
		portByName.put(face1, port);
		portByName.put(face2, port);
		portById.put(1, port);
	}
	
	public boolean initialize() {
		
		Session hbnSession = null;
		boolean success = false;
		try {
			hbnSession = OrkTrack.hibernateManager.getSession();
			Transaction tx = hbnSession.beginTransaction();
			
			Iterator portFaces = hbnSession.createQuery(
        	"from OrkPortFace")
        	.list()
        	.iterator();

			while ( portFaces.hasNext() ) {
			    OrkPortFace portFace = (OrkPortFace)portFaces.next();
			    
			    int portId = portFace.getPort().getId();
			    Port port = portById.get(portId);
			    if(port == null) {
			    	OrkPort recPort = (OrkPort)hbnSession.get(OrkPort.class, portId);
			    	if (recPort != null) {
				    	port = new Port(recPort);
				    	portById.put(portId, port);
			    	}
			    }
		    	port.portFaces.add(portFace);
		    	portByName.put(portFace.getName(), port);
			}
			tx.commit();
			success = true;
		}
		catch (Exception e) {
			logger.error("initialize: exception:" + e.getClass().getName());
		}
		finally {
			if(hbnSession != null) {hbnSession.close();}
		}
		return success;
	}
	
	public OrkPort getRecPortByFace(String face, Session hbnSession) {
		OrkPort port = null;
		List ports = hbnSession.createQuery(
	    "from OrkPortFace as pf join pf.port as p where pf.name=:face")
	    .setString("face", face)
	    .list();
		if (ports.size() > 0) {
			Object[] row =  (Object[])ports.get(0);
			if (row.length > 1) {
				port = (OrkPort)row[1];
			}
		}
		return port;
	}
}
