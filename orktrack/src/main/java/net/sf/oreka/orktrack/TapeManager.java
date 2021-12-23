package net.sf.oreka.orktrack;

import net.sf.oreka.orktrack.messages.TapeMessage;
import net.sf.oreka.persistent.OrkSegment;
import net.sf.oreka.persistent.OrkService;
import net.sf.oreka.persistent.OrkTape;
import net.sf.oreka.persistent.OrkUser;
import org.apache.log4j.Logger;
import org.hibernate.Session;
import org.hibernate.query.Query;

import java.util.Date;
import java.util.Optional;


public class TapeManager {
	
	private Logger logger = Logger.getLogger(TapeManager.class);
	
	static TapeManager tapeManager = null;
	
	private TapeManager () {
	}
	
	public static TapeManager instance() {
		if (tapeManager == null) {
			tapeManager = new TapeManager();
		}
		return tapeManager;
	}

	public Optional<OrkTape> getTapeByNativeCallID(Session hbnSession, String callId) {
		Query<OrkTape> query = hbnSession.createQuery("from OrkTape where nativeCallId=:nativeCallId order by id desc", OrkTape.class);
		query.setParameter("nativeCallId", callId);
		query.setMaxResults(1);
		return query.uniqueResultOptional();
	}

	public Optional<OrkTape> getBestMatchingRunningTape(Session hbnSession, TapeMessage tapeMessage) {
		Query<OrkTape> query = hbnSession.createQuery("from OrkTape where portName=:portName and nativeCallId=:nativeCallId and DATEDIFF(timestamp, NOW()) < :timeOffSet order by id desc", OrkTape.class);
		query.setParameter("portName", tapeMessage.getCapturePort());
		query.setParameter("nativeCallId", tapeMessage.getNativeCallId());

		if (!tapeMessage.getNativeCallId().equals("")){
			query.setParameter("timeOffSet", 7);
		} else {
			query.setParameter("timeOffSet", 1);
		}
		query.setMaxResults(1);
		return query.uniqueResultOptional();
	}
	
	/**
	 * @param tapeMessage
	 * @param hbnSession
	 * @param srv
	 * @return	false if the tape is rejected and should be deleted, otherwise true
	 */
	public boolean notifyTapeMessage(TapeMessage tapeMessage, Session hbnSession, OrkService srv) {

		boolean keepTape = true;
		long date = ((long)tapeMessage.getTimestamp()) * 1000;
		Date timestamp = new Date(date);

		// create a new tape record
		OrkTape recTape = new OrkTape();
		recTape.setDirection(tapeMessage.getDirection());
		recTape.setDuration(tapeMessage.getDuration());
		recTape.setExpiryTimestamp(new Date());
		recTape.setLocalParty(tapeMessage.getLocalParty());
		recTape.setPortName(tapeMessage.getCapturePort());
		recTape.setRemoteParty(tapeMessage.getRemoteParty());
		recTape.setTimestamp(timestamp);
		recTape.setNativeCallId(tapeMessage.getNativeCallId());
		recTape.setState(tapeMessage.getStage().name());
		recTape.setService(srv);

		if (tapeMessage.getStage() == TapeMessage.CaptureStage.START) {
			//also insert into db for tracking
			// create a new tape record
			recTape.setFilename(tapeMessage.getFilename()+".mcf");
			hbnSession.save(recTape);
			logger.info("Written start tape:" + tapeMessage.getRecId() + " as " + recTape.getId());
		} else if (tapeMessage.getStage() == TapeMessage.CaptureStage.STOP) {
			//Update if present
			Optional<OrkTape> existingTape  = getBestMatchingRunningTape(hbnSession, tapeMessage);
			if (existingTape.isPresent()){
				recTape = existingTape.get();
				recTape.setDuration(tapeMessage.getDuration());
				recTape.setLocalParty(tapeMessage.getLocalParty());
				recTape.setRemoteParty(tapeMessage.getRemoteParty());
				recTape.setFilename(tapeMessage.getFilename());
				recTape.setState(tapeMessage.getStage().name());
				hbnSession.update(recTape);
				logger.info("Updated stop tape:" + tapeMessage.getRecId() + " as " + recTape.getId());
			}
		} else if (tapeMessage.getStage() == TapeMessage.CaptureStage.READY){
			// Tape stop message
			//Retrieve Existing Tape if any
			String currentCallState = null;
			 Optional<OrkTape> existingTape  = getBestMatchingRunningTape(hbnSession, tapeMessage);
			if (existingTape.isPresent()){
				recTape = existingTape.get();
				currentCallState = recTape.getState();
				recTape.setFilename(tapeMessage.getFilename());
				recTape.setState(tapeMessage.getStage().name());

				hbnSession.update(recTape);
				logger.info("Updated ready tape:" + tapeMessage.getRecId() + " as " + recTape.getId());
			} else {
				recTape.setFilename(tapeMessage.getFilename());
				hbnSession.save(recTape);
				logger.info("Added ready tape:" + tapeMessage.getRecId() + " as " + recTape.getId());
			}

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
