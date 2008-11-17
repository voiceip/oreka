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

import net.sf.oreka.Direction;
import net.sf.oreka.OrkException;
import net.sf.oreka.messages.AsyncMessage;
import net.sf.oreka.messages.SimpleResponseMessage;
import net.sf.oreka.messages.SyncMessage;
import net.sf.oreka.orktrack.LogManager;
import net.sf.oreka.orktrack.OrkTrack;
import net.sf.oreka.orktrack.ServiceManager;
import net.sf.oreka.orktrack.TapeManager;
import net.sf.oreka.persistent.OrkService;
import net.sf.oreka.serializers.OrkSerializer;
import net.sf.oreka.serializers.SingleLineSerializer;

import org.apache.log4j.Logger;
import org.hibernate.Session;
import org.hibernate.Transaction;

public class TapeMessage extends SyncMessage {

	public enum CaptureStage {START , STOP, COMPLETE, UNKN};
	
	static Logger logger = Logger.getLogger(TapeMessage.class);
	
	CaptureStage stage = CaptureStage.UNKN;
	int timestamp = 0;
	int duration = 0;
	String recId = "";
	String filename = "";
	String captureId = "";
	String capturePort = "";
	String localParty = "";
	String localEntryPoint = "";
	String remoteParty = "";
	Direction direction = Direction.UNKN;
	String loginString = "";
	String service = "";
	String srcIp = "";
	String dstIp = "";
	int srcTcpPort = 0;
	int dstTcpPort = 0;
	String srcMac = "";
	String dstMac = "";
	
	public TapeMessage() {
	}
	
	public String getService() {
		return service;
	}

	public void setService(String service) {
		this.service = service;
	}

	@Override
	public AsyncMessage process() {

		TapeResponse response = new TapeResponse();
		Session session = null;
		Transaction tx = null;
		
		try {
			session = OrkTrack.hibernateManager.getSession();
	        tx = session.beginTransaction();
	        
	        SingleLineSerializer ser = new SingleLineSerializer();
	        logger.info("Message: " + ser.serialize(this));
	        
			OrkService service = ServiceManager.retrieveOrCreate(this.service, this.getHostname(), session);
			
			//Port port = PortManager.instance().getAndCreatePort(this.getCapturePort(), session, service);
			//port.notifyTapeMessage(this, session, service);
			if (TapeManager.instance().notifyTapeMessage(this, session, service) == false) {
				response.setDeleteTape(true);
				logger.debug("Tape deletion requested:" + this.getFilename());
			}
			
			response.setSuccess(true);
			tx.commit();
		}
		catch (Exception e) {
			logger.error("TapeMessage.process: ", e);
			response.setSuccess(false);
			response.setComment(e.getMessage());
		}
		finally {
			if(session != null) {session.close();}
		}
		return response;
	}

	public void define(OrkSerializer serializer) throws OrkException {
		
		defineMessage(serializer);
		
		recId = serializer.stringValue("recid", recId, false);
		stage = (CaptureStage)serializer.enumValue("stage", stage, true);
		timestamp = serializer.intValue("timestamp", timestamp, true);
		duration = serializer.intValue("duration", duration, true);		
		filename = serializer.stringValue("filename", filename, true);
		capturePort = serializer.stringValue("captureport", capturePort, true);
		localParty = serializer.stringValue("localparty", localParty, false);
		localEntryPoint = serializer.stringValue("localentrypoint", localEntryPoint, false);
		remoteParty = serializer.stringValue("remoteparty", remoteParty, false);
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

	public String getCaptureId() {
		return captureId;
	}
	

	public void setCaptureId(String captureId) {
		this.captureId = captureId;
	}
	

	public String getDstIp() {
		return dstIp;
	}
	

	public void setDstIp(String dstIp) {
		this.dstIp = dstIp;
	}
	

	public String getDstMac() {
		return dstMac;
	}
	

	public void setDstMac(String dstMac) {
		this.dstMac = dstMac;
	}
	

	public int getDstTcpPort() {
		return dstTcpPort;
	}
	

	public void setDstTcpPort(int dstTcpPort) {
		this.dstTcpPort = dstTcpPort;
	}
	

	public String getSrcIp() {
		return srcIp;
	}
	

	public void setSrcIp(String srcIp) {
		this.srcIp = srcIp;
	}
	

	public String getSrcMac() {
		return srcMac;
	}
	

	public void setSrcMac(String srcMac) {
		this.srcMac = srcMac;
	}
	

	public int getSrcTcpPort() {
		return srcTcpPort;
	}
	

	public void setSrcTcpPort(int srcTcpPort) {
		this.srcTcpPort = srcTcpPort;
	}

	public String getRecId() {
		return recId;
	}
	

	public void setRecId(String recId) {
		this.recId = recId;
	}
	
	

}
