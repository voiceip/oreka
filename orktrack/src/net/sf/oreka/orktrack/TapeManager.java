package net.sf.oreka.orktrack;

import java.util.Date;

import net.sf.oreka.orktrack.messages.TapeMessage;
import net.sf.oreka.persistent.RecSegment;
import net.sf.oreka.persistent.RecTape;
import net.sf.oreka.persistent.Service;
import net.sf.oreka.persistent.User;

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
	
	
	public void notifyTapeMessage(TapeMessage tapeMessage, Session hbnSession, Service srv) {
		
		if (tapeMessage.getStage() == TapeMessage.CaptureStage.START) {
			; // tape start message
		}
		else {
			// Tape stop message
			long date = ((long)tapeMessage.getTimestamp()) * 1000;
			Date timestamp = new Date(date);
            
			// create a new tape record
			RecTape recTape = new RecTape();
			recTape.setDirection(tapeMessage.getDirection());
			recTape.setDuration(tapeMessage.getDuration());
			recTape.setExpiryTimestamp(new Date());
			recTape.setFilename(tapeMessage.getFilename());
			recTape.setLocalParty(tapeMessage.getLocalParty());
			recTape.setRecPortName(tapeMessage.getCapturePort());
			recTape.setRemoteParty(tapeMessage.getRemoteParty());
			recTape.setTimestamp(timestamp);
			recTape.setService(srv);
			hbnSession.save(recTape);
			logger.info("Written tape:" + tapeMessage.getRecId() + " as " + recTape.getId());
			
			RecSegment recSegment = new RecSegment();
			recSegment.setTimestamp(timestamp);
			recSegment.setDirection(tapeMessage.getDirection());
			recSegment.setDuration(tapeMessage.getDuration());
			recSegment.setRemoteParty(tapeMessage.getRemoteParty());
			recSegment.setLocalParty(tapeMessage.getLocalParty());
			recSegment.setLocalEntryPoint(tapeMessage.getLocalEntryPoint());
			recSegment.setRecTape(recTape);
			recSegment.setRecPortName(recTape.getRecPortName());
			
			if(tapeMessage.getLocalParty() != "") {
				User user = UserManager.instance().getByLoginString(tapeMessage.getLocalParty(), hbnSession);
				recSegment.setUser(user);
			}
			if (ProgramManager.instance().filterSegmentAgainstAllPrograms(recSegment, hbnSession)) {
				hbnSession.save(recSegment);
				logger.info("Written segment:" + tapeMessage.getRecId() + " as " + recSegment.getId());
			}
			else {
				logger.info("Tape:" + tapeMessage.getRecId() + " generates no segment");					
			}
		}
	}
}
