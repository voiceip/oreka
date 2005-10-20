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

import net.sf.oreka.HibernateManager;
import net.sf.oreka.persistent.RecPort;
import net.sf.oreka.persistent.RecPortFace;
import net.sf.oreka.persistent.Service;

import org.apache.log4j.Logger;
import org.hibernate.Session;
import org.hibernate.Transaction;

/** Singleton class that manages activity on all capture ports */
public class PortManager {

	HashMap<Integer, Port> portById = new HashMap<Integer, Port>();
	HashMap<String, Port> portByName = new HashMap<String, Port>();
	
	static PortManager portManager = null;
	Logger log = null;
	
	private PortManager () {
		log = LogManager.getInstance().portLogger;
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
	
	public Port getAndCreatePort(String name, Session hbnSession, Service service) {
		
		Port port = portByName.get(name);
		if (port == null) {
			
			port = createPort(name, hbnSession, service);
		}
		return port; 
	}
	
	public synchronized Port createPort(String name, Session hbnSession, Service service) {
		
		RecPort recPort = null;

		RecPortFace portFace = (RecPortFace)hbnSession.get(RecPortFace.class, name);
		if (portFace == null) {
			portFace  = new RecPortFace();
			portFace.setName(name);
			portFace.setRecPort(recPort);
			portFace.setService(service);
			
			recPort = new RecPort();
			portFace.setRecPort(recPort);
		}
		else {
			portFace.setRecPort(recPort);
			portFace.setService(service);
			recPort = portFace.getRecPort();
		}
		
		hbnSession.save(recPort);
		hbnSession.save(portFace);
		
		log.info("created port:" + recPort.getId() + " with face:" + name);
		
    	Port port = new Port(recPort);
    	port.portFaces.add(portFace);
    	portById.put(recPort.getId(), port);
    	portByName.put(portFace.getName(), port);
		return port;
	}
	
	// for testing purposes
	public void addPort(String face1, String face2, Session hbnSession) {
		
		RecPort recPort = new RecPort();
		Port port = new Port(recPort);
		RecPortFace portFace1 = new RecPortFace();
		portFace1.setName(face1);
		portFace1.setRecPort(recPort);
		RecPortFace portFace2 = new RecPortFace();
		portFace2.setName(face2);
		portFace2.setRecPort(recPort);
		
		hbnSession.save(recPort);
		hbnSession.save(portFace1);
		hbnSession.save(portFace2);
		
		port.portFaces.add(portFace1);
		port.portFaces.add(portFace2);	
		portByName.put(face1, port);
		portByName.put(face2, port);
		portById.put(1, port);
	}
	
	public void initialize() {
		
		try {
			Session session = HibernateManager.getSession();
			Transaction tx = session.beginTransaction();
			
			Iterator portFaces = session.createQuery(
        	"from RecPortFace")
        	.list()
        	.iterator();

			while ( portFaces.hasNext() ) {
			    RecPortFace portFace = (RecPortFace)portFaces.next();
			    
			    int portId = portFace.getRecPort().getId();
			    Port port = portById.get(portId);
			    if(port == null) {
			    	RecPort recPort = (RecPort)session.get(RecPort.class, portId);
			    	if (recPort != null) {
				    	port = new Port(recPort);
				    	portById.put(portId, port);
			    	}
			    }
		    	port.portFaces.add(portFace);
		    	portByName.put(portFace.getName(), port);
			}
			tx.commit();
			session.close();
		}
		catch (Exception e) {
			log.warn("HibernateManager.initialize: could not initialize", e);
		}
	}
	
	public RecPort getRecPortByFace(String face, Session hbnSession) {
		RecPort port = null;
		List ports = hbnSession.createQuery(
	    "from RecPortFace as pf join pf.recPort as p where pf.name=:face")
	    .setString("face", face)
	    .list();
		if (ports.size() > 0) {
			Object[] row =  (Object[])ports.get(0);
			if (row.length > 1) {
				port = (RecPort)row[1];
			}
		}
		return port;
	}
}
