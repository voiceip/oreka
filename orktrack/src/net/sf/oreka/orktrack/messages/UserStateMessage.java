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

package net.sf.oreka.orktrack.messages;

import net.sf.oreka.OrkException;
import net.sf.oreka.HibernateManager;
import net.sf.oreka.messages.AsyncMessage;
import net.sf.oreka.messages.SimpleResponseMessage;
import net.sf.oreka.messages.SyncMessage;
import net.sf.oreka.orktrack.LogManager;
import net.sf.oreka.orktrack.OrkTrack;
import net.sf.oreka.orktrack.Port;
import net.sf.oreka.orktrack.PortManager;
import net.sf.oreka.orktrack.UserManager;
import net.sf.oreka.persistent.OrkUser;
import net.sf.oreka.serializers.OrkSerializer;

import org.apache.log4j.Logger;
import org.hibernate.Session;
import org.hibernate.Transaction;

public class UserStateMessage extends SyncMessage {

	public enum UserState {LOGIN, LOGOUT, UNKN};
	
	private Logger log = null;
	
	private UserState userState = UserState.UNKN;
	private String loginString = "";
	private String recPort = "";
	
	public UserStateMessage() {
		log = LogManager.getInstance().getPortLogger();
	}
	
	@Override
	public AsyncMessage process() {
		
		SimpleResponseMessage response = new SimpleResponseMessage();
		
		Session hbnSession = null;
		Transaction tx = null;
		try {
			String comment = "";
			boolean success = false;
			hbnSession = OrkTrack.hibernateManager.getSession();
	        tx = hbnSession.beginTransaction();
	        // find out user
	        OrkUser user = UserManager.instance().getByLoginString(loginString, hbnSession);
	        if (user == null) {
	        	comment = "Could not find user for login string:" + loginString;
	        	log.warn(comment);
	        }
	        else {
	        	// find out port
	        	Port port = PortManager.instance().getPort(recPort);
	        	if (port == null) {
		        	comment = "Could not find port for face:" + recPort;
		        	log.warn(comment);
	        	}
	        	else {
	        		if(userState == UserState.LOGIN) {
	        			port.setUser(user);
	        			UserManager.instance().setUserLocation(user, port);
	        			log.info("user:" + loginString + " logging onto:" + recPort);
	        		}
	        		else if (userState == UserState.LOGOUT) {
	        			port.setUser(null);
	        			UserManager.instance().setUserLocation(user, null);
	        			log.info("user:" + loginString + " logging out of:" + recPort);
	        		}
	        		success = true;
	        	}
	        }

	        response.setComment(comment);
			response.setSuccess(success);
			tx.commit();
		}
		catch (Exception e) {
			log.error("TapeMessage.process: ", e);
			response.setSuccess(false);
			response.setComment(e.getMessage());
		}
		finally {
			if(hbnSession != null) {hbnSession.close();}
		}
		
		return response;
	}

	public void define(OrkSerializer serializer) throws OrkException {
		
		userState = (UserState)serializer.enumValue("userstate", userState, true);
		loginString = serializer.stringValue("loginstring", loginString, true);
		recPort = serializer.stringValue("recport", recPort, true);
	}

	public String getOrkClassName() {
		return "userstate";
	}

	public void validate() {
		// TODO Auto-generated method stub
		
	}

}
