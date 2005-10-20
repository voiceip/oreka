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
import net.sf.oreka.Direction;
import net.sf.oreka.HibernateManager;
import net.sf.oreka.messages.AsyncMessage;
import net.sf.oreka.messages.SimpleResponseMessage;
import net.sf.oreka.messages.SyncMessage;
import net.sf.oreka.orktrack.LogManager;
import net.sf.oreka.orktrack.Port;
import net.sf.oreka.orktrack.PortManager;
import net.sf.oreka.orktrack.ServiceManager;
import net.sf.oreka.persistent.Service;
import net.sf.oreka.serializers.OrkSerializer;
import net.sf.oreka.serializers.SingleLineSerializer;

import org.apache.log4j.Logger;
import org.hibernate.Session;
import org.hibernate.Transaction;

public class TapeMessage extends SyncMessage {

	public enum CaptureStage {start , stop, complete, unkn};
	
	Logger log = null;
	
	CaptureStage stage = CaptureStage.unkn;
	int timestamp = 0;
	int duration = 0;
	String filename = "";
	String capturePort = "";
	String localParty = "";
	String localEntryPoint = "";
	String remoteParty = "";
	Direction direction = Direction.UNKN;
	String loginString = "";
	String service = "";
	
	public TapeMessage() {
		log = LogManager.getInstance().getPortLogger();
	}
	
	public String getService() {
		return service;
	}

	public void setService(String service) {
		this.service = service;
	}

	@Override
	public AsyncMessage process() {

		SimpleResponseMessage response = new SimpleResponseMessage();
		Session session = null;
		Transaction tx = null;
		
		try {
			session = HibernateManager.getSession();
	        tx = session.beginTransaction();
	        
	        SingleLineSerializer ser = new SingleLineSerializer();
	        log.info("Message: " + ser.serialize(this));
	        
			Service service = ServiceManager.retrieveOrCreate(this.service, session);
			
			Port port = PortManager.instance().getAndCreatePort(this.getCapturePort(), session, service);
			port.notifyTapeMessage(this, session, service);
			
			response.setSuccess(true);
			tx.commit();
		}
		catch (Exception e) {
			if (tx != null) {tx.rollback();}
			log.error("TapeMessage.process: ", e);
			response.setSuccess(false);
			response.setComment(e.getMessage());
		}
		finally {
			if(session != null) {session.close();}
		}
		return response;
	}

	public void define(OrkSerializer serializer) throws OrkException {
		
		stage = (CaptureStage)serializer.enumValue("stage", stage, true);
		timestamp = serializer.intValue("timestamp", timestamp, true);
		duration = serializer.intValue("duration", duration, true);		
		filename = serializer.stringValue("filename", filename, true);
		capturePort = serializer.stringValue("capturePort", capturePort, true);
		localParty = serializer.stringValue("localParty", localParty, false);
		localEntryPoint = serializer.stringValue("localEntryPoint", localEntryPoint, false);
		remoteParty = serializer.stringValue("remoteParty", remoteParty, false);
		direction = (Direction)serializer.enumValue("direction", direction, false);
		loginString = serializer.stringValue("loginString", loginString, false);
		service = serializer.stringValue("service", service, true);
	}

	public String getOrkClassName() {
		return "tape";
	}

	public void validate() {
		// TODO Auto-generated method stub
		
	}

	public String getCapturePort() {
		return capturePort;
	}

	public void setCapturePort(String capturePort) {
		this.capturePort = capturePort;
	}

	public Direction getDirection() {
		return direction;
	}

	public void setDirection(Direction direction) {
		this.direction = direction;
	}

	public String getFilename() {
		return filename;
	}

	public void setFilename(String filename) {
		this.filename = filename;
	}

	public String getLocalEntryPoint() {
		return localEntryPoint;
	}

	public void setLocalEntryPoint(String localEntryPoint) {
		this.localEntryPoint = localEntryPoint;
	}

	public String getLocalParty() {
		return localParty;
	}

	public void setLocalParty(String localParty) {
		this.localParty = localParty;
	}

	public String getLoginString() {
		return loginString;
	}

	public void setLoginString(String loginString) {
		this.loginString = loginString;
	}

	public String getRemoteParty() {
		return remoteParty;
	}

	public void setRemoteParty(String remoteParty) {
		this.remoteParty = remoteParty;
	}

	public CaptureStage getStage() {
		return stage;
	}

	public void setStage(CaptureStage stage) {
		this.stage = stage;
	}

	public int getTimestamp() {
		return timestamp;
	}

	public void setTimestamp(int timestamp) {
		this.timestamp = timestamp;
	}

	public int getDuration() {
		return duration;
	}

	public void setDuration(int duration) {
		this.duration = duration;
	}

}
