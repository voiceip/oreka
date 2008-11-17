package net.sf.oreka.orktrack;

import java.util.Date;

import net.sf.oreka.orktrack.messages.TapeMessage;
import net.sf.oreka.persistent.OrkSegment;
import net.sf.oreka.persistent.OrkTape;
import net.sf.oreka.persistent.OrkService;
import net.sf.oreka.persistent.OrkUser;

import org.apache.log4j.Logger;
import org.hibernate.Session;

public class TapeManager {
	
	static Logger logger = Logger.getLogger(TapeManager.class);
	
	static TapeManager tapeManager = null;
	
	private TapeManager () {
	}
	
	public static TapeManager instance() {
		if (tapeManager == null) {
			tapeManager = new TapeManager();
		}
		return tapeManager;
	}
	
	/**
	 * @param tapeMessage
	 * @param hbnSession
	 * @param srv
	 * @return	false if the tape is rejected and should be deleted, otherwise true
	 */
	public boolean notifyTapeMessage(TapeMessage tapeMessage, Session hbnSession, OrkService srv) {
		
		boolean keepTape = true;
		
		if (tapeMessage.getStage() == TapeMessage.CaptureStage.START) {
			; // tape start message
		}
		else {
			// Tape stop message
			long date = ((long)tapeMessage.getTimestamp()) * 1000;
			Date timestamp = new Date(date);
            
			// create a new tape record
			OrkTape recTape = new OrkTape();
			recTape.setDirection(tapeMessage.getDirection());
			recTape.setDuration(tapeMessage.getDuration());
			recTape.setExpiryTimestamp(new Date());
			recTape.setFilename(tapeMessage.getFilename());
			recTape.setLocalParty(tapeMessage.getLocalParty());
			recTape.setPortName(tapeMessage.getCapturePort());
			recTape.setRemoteParty(tapeMessage.getRemoteParty());
			recTape.setTimestamp(timestamp);
			recTape.setService(srv);
			hbnSession.save(recTape);
			logger.info("Written tape:" + tapeMessage.getRecId() + " as " + recTape.getId());
			
			OrkSegment recSegment = new OrkSegment();
			recSegment.setTimestamp(timestamp);
			recSegment.setDirection(tapeMessage.getDirection());
			recSegment.setDuration(tapeMessage.getDuration());
			recSegment.setRemoteParty(tapeMessage.getRemoteParty());
			recSegment.setLocalParty(tapeMessage.getLocalParty());
			recSegment.setLocalEntryPoint(tapeMessage.getLocalEntryPoint());
			recSegment.setTape(recTape);
			recSegment.setPortName(recTape.getPortName());
			
			if(tapeMessage.getLocalParty() != "") {
				OrkUser user = UserManager.instance().getByLoginString(tapeMessage.getLocalParty(), hbnSession);
				recSegment.setUser(user);
			}
			if (ProgramManager.instance().filterSegmentAgainstAllPrograms(recSegment, hbnSession)) {
				hbnSession.save(recSegment);
				logger.info("Written segment:" + tapeMessage.getRecId() + " as " + recSegment.getId());
			}
			else {
				logger.info("Tape:" + tapeMessage.getRecId() + " not retained by any program");
				keepTape = false;
			}
		}
		return keepTape;
	}
}
