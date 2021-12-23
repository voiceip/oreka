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

import com.codahale.metrics.SharedMetricRegistries;
import com.codahale.metrics.annotation.Timed;
import io.astefanutti.metrics.aspectj.Metrics;
import lombok.Getter;
import lombok.Setter;
import net.sf.oreka.Direction;
import net.sf.oreka.OrkException;
import net.sf.oreka.messages.AsyncMessage;
import net.sf.oreka.messages.SyncMessage;
import net.sf.oreka.orktrack.Constants;
import net.sf.oreka.orktrack.OrkTrack;
import net.sf.oreka.orktrack.ServiceManager;
import net.sf.oreka.orktrack.TapeManager;
import net.sf.oreka.persistent.OrkService;
import net.sf.oreka.serializers.OrkSerializer;
import net.sf.oreka.serializers.SingleLineSerializer;

import org.apache.log4j.Logger;
import org.hibernate.Session;
import org.hibernate.Transaction;

@Getter
@Setter
@Metrics
public class TapeMessage extends SyncMessage {

	public  enum CaptureStage {START , STOP, READY, COMPLETE, UNKN};
	
	static Logger log = Logger.getLogger(TapeMessage.class);
	
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
	String nativeCallId = "";
	
	public TapeMessage() {
	}

	@Override
	@Timed(name = "process")
	public AsyncMessage process() {

        SharedMetricRegistries.getOrCreate(Constants.DEFAULT_REGISTRY_NAME)
                .meter(getClass().getName()+".processing."+stage).mark();

        TapeResponse response = new TapeResponse();
		Session session = null;

		try {
			session = OrkTrack.hibernateManager.getSession();
			Transaction tx = session.beginTransaction();

	        SingleLineSerializer ser = new SingleLineSerializer();
	        log.info("Message: " + ser.serialize(this));
	        
			OrkService service = ServiceManager.retrieveOrCreate(this.service, this.getHostname(), session);
			
			//Port port = PortManager.instance().getAndCreatePort(this.getCapturePort(), session, service);
			//port.notifyTapeMessage(this, session, service);
			if (TapeManager.instance().notifyTapeMessage(this, session, service) == false) {
				response.setDeleteTape(true);
				log.debug("Tape deletion requested:" + this.getFilename());
			}
			
			response.setSuccess(true);
			tx.commit();
		}
		catch (Exception e) {
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
		nativeCallId = serializer.stringValue("nativecallid", nativeCallId, false);
	}

	public String getOrkClassName() {
		return "tape";
	}

	public void validate() {
		// TODO Auto-generated method stub

	}
	

}
