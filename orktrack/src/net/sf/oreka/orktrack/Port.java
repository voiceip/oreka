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

import java.util.ArrayList;
import java.util.Date;
import java.util.Iterator;
import java.util.LinkedList;

import net.sf.oreka.orktrack.messages.MetadataMessage;
import net.sf.oreka.orktrack.messages.TapeMessage;
import net.sf.oreka.persistent.OrkPort;
import net.sf.oreka.persistent.OrkPortFace;
import net.sf.oreka.persistent.OrkSegment;
import net.sf.oreka.persistent.OrkTape;
import net.sf.oreka.persistent.OrkService;
import net.sf.oreka.persistent.OrkUser;

import org.apache.log4j.Logger;
import org.hibernate.Session;

public class Port {

	public ArrayList<OrkPortFace> portFaces = new ArrayList<OrkPortFace>();
	private OrkPort recPort = null;
	private TapeMessage lastTapeMessage = null;
	private MetadataMessage lastMetadataMessage = null;
	private OrkUser user = null;
	
	private LinkedList<OrkSegment> recSegments = new LinkedList<OrkSegment> ();
	private LinkedList<OrkTape> recTapes = new LinkedList<OrkTape> ();
	
	static Logger logger = Logger.getLogger(ProgramManager.class);
	
	public Port(OrkPort recPort) {
		this.recPort = recPort;
	}
	
	private synchronized void addTapeToLocalCache(OrkTape tape) {
		
		if (recTapes.size() > 50) {
			recTapes.removeLast();
		}
		recTapes.addFirst(tape);
	}
	
	private synchronized void addSegmentToLocalCache(OrkSegment seg) {
		
		if (recSegments.size() > 50) {
			recSegments.removeLast();
		}
		recSegments.addFirst(seg);
	}
	
	private boolean areTogether(OrkTape tape, OrkSegment seg) {
		
		boolean result = false;
		long segStartTime = seg.getTimestamp().getTime();
		long tapeStartTime = tape.getTimestamp().getTime();
		long segStopTime = segStartTime + seg.getDuration();
		long tapeStopTime = tapeStartTime + tape.getDuration();
		
		if(	segStartTime > tapeStartTime || 
			(tapeStartTime - segStartTime) < 3000 ) {
		
			if (	segStopTime < tapeStopTime ||
					(segStopTime-tapeStopTime) < 3000 ) {
				
				result = true;
			}
		}
		return result;
	}
	
	private void joinTogether(OrkTape tape, OrkSegment seg, Session hbnSession) {

		seg.setTape(tape);
		seg.setTapeOffset(seg.getTimestamp().getTime() - tape.getTimestamp().getTime());
		hbnSession.save(seg);
	}
	
	private synchronized void findAndAssociateTape(OrkSegment segment, Session hbnSession) {
		
		// Iterate over tapes and find the ones corresponding to this segment
		Iterator<OrkTape> it = recTapes.iterator();
		boolean foundOne = false;
		boolean done = false;
		while (it.hasNext() && !done) {
			OrkTape tape = it.next();
			if (areTogether(tape, segment)) {
				joinTogether(tape, segment, hbnSession);
				logger.info("#" + recPort.getId() + ": associating segment:" + segment.getId() + " with tape:" + tape.getId());
				foundOne = true;
			}
			else {
				if (foundOne) {
					done = true;
				}
			}
		}
	}
	
	private synchronized void findAndAssociateSegment(OrkTape tape, Session hbnSession) {
		
		// Iterate over segments and find the ones corresponding to this tape
		Iterator<OrkSegment> it = recSegments.iterator();
		boolean foundOne = false;
		boolean done = false;
		while (it.hasNext() && !done) {
			OrkSegment segment = it.next();
			if (areTogether(tape, segment)) {
				joinTogether(tape, segment, hbnSession);
				logger.info("#" + recPort.getId() + ": associating segment:" + segment.getId() + " with tape:" + tape.getId());
				foundOne = true;
			}
			else {
				if (foundOne) {
					done = true;
				}
			}
		}
	}
	
	public void notifyMetadataMessage(MetadataMessage metadataMessage, Session hbnSession, OrkService srv) {
		
		long duration = 0;
		Date startDate = null;
		
		if (metadataMessage.getStage() == TapeMessage.CaptureStage.START) {
			lastMetadataMessage = metadataMessage;
		}
		else if (metadataMessage.getStage() == TapeMessage.CaptureStage.STOP) {
			
			if (lastMetadataMessage != null) {
				duration = ((long)metadataMessage.getTimestamp() - (long)lastMetadataMessage.getTimestamp())*1000;
				startDate = new Date((long)lastMetadataMessage.getTimestamp()*1000);
			}
		}
		else if (metadataMessage.getStage() == TapeMessage.CaptureStage.COMPLETE) {
			duration = (long)metadataMessage.getDuration()*1000;
			startDate = new Date(metadataMessage.getTimestamp()*1000);
		}
		
		if (startDate != null) {
			
			if (srv.isRecordMaster()) {
				// create segment
				OrkSegment recSegment = new OrkSegment();
				recSegment.setTimestamp(startDate);
				recSegment.setDirection(metadataMessage.getDirection());
				recSegment.setDuration(duration);
				recSegment.setLocalParty(metadataMessage.getLocalParty());
				recSegment.setLocalEntryPoint(metadataMessage.getLocalEntryPoint());
				recSegment.setSessionOffset(0);
				recSegment.setPort(recPort);
				
				if(metadataMessage.getLocalParty() != "") {
					OrkUser user = UserManager.instance().getByLoginString(metadataMessage.getLocalParty(), hbnSession);
					recSegment.setUser(user);
				}
				
				// is this retained by a program ?
				if (ProgramManager.instance().filterSegmentAgainstAllPrograms(recSegment, hbnSession)) {
					// Try to find associated tape and link it to this
					addSegmentToLocalCache(recSegment);
					findAndAssociateTape(recSegment, hbnSession);
					hbnSession.save(recSegment);
					logger.info("#" + metadataMessage.getCapturePort() + ": written segment " + recSegment.getId());
				}
				else {
					logger.info("#" + metadataMessage.getCapturePort() + ":  metadata message discarded");
				}
			}
		}
	}
	
	public void notifyTapeMessage(TapeMessage tapeMessage, Session hbnSession, OrkService srv) {
		
		if (tapeMessage.getStage() == TapeMessage.CaptureStage.START) {
			lastTapeMessage = tapeMessage;
		}
		else {
			// Tape stop
			if (lastTapeMessage == null) {
				logger.warn("Port: notifyTapeMessage: stop without a previous tape message");
			}
			else if (lastTapeMessage.getStage() != TapeMessage.CaptureStage.START) {
				logger.warn("Port: notifyTapeMessage: stop without a start");	
			}
			else if (tapeMessage.getStage() == TapeMessage.CaptureStage.STOP){
				boolean generateSegment = false;
				if (portFaces.size() == 1 || srv.isRecordMaster()) {
					generateSegment = true;
				}
				
				TapeMessage startMessage = lastTapeMessage;
				TapeMessage stopMessage = tapeMessage;
				//long duration = (stopMessage.getTimestamp() - startMessage.getTimestamp())*1000;
				long duration = stopMessage.getDuration();
				long date = ((long)startMessage.getTimestamp()) * 1000;
				Date timestamp = new Date(date);
	            
				// create a new tape record
				OrkTape recTape = new OrkTape();
				recTape.setDirection(stopMessage.getDirection());
				recTape.setDuration(duration);
				recTape.setExpiryTimestamp(new Date());
				recTape.setFilename(stopMessage.getFilename());
				recTape.setLocalParty(stopMessage.getLocalParty());
				recTape.setPort(recPort);
				recTape.setRemoteParty(stopMessage.getRemoteParty());
				recTape.setTimestamp(timestamp);
				recTape.setService(srv);
				hbnSession.save(recTape);
				logger.info("#" + tapeMessage.getCapturePort() + ": written tape " + recTape.getId());
				
				if (generateSegment) {
					OrkSegment recSegment = new OrkSegment();
					recSegment.setTimestamp(timestamp);
					recSegment.setDirection(stopMessage.getDirection());
					recSegment.setDuration(duration);
					recSegment.setRemoteParty(stopMessage.getRemoteParty());
					recSegment.setLocalParty(stopMessage.getLocalParty());
					recSegment.setLocalEntryPoint(stopMessage.getLocalEntryPoint());
					recSegment.setTape(recTape);
					recSegment.setPort(recPort);
					
					if(stopMessage.getLocalParty() != "") {
						OrkUser user = UserManager.instance().getByLoginString(stopMessage.getLocalParty(), hbnSession);
						recSegment.setUser(user);
					}
					if (ProgramManager.instance().filterSegmentAgainstAllPrograms(recSegment, hbnSession)) {
						hbnSession.save(recSegment);
						logger.info("#" + tapeMessage.getCapturePort() + ": written segment " + recSegment.getId());
					}
					else {
						logger.info("#" + tapeMessage.getCapturePort() + ":  tape message discarded");					}
				}
				else {
					// segments are generated by metadata messages
					addTapeToLocalCache(recTape);
					findAndAssociateSegment(recTape, hbnSession);
				}
			}
		}
	}

	public OrkPort getRecPort() {
		return recPort;
	}

	public void setRecPort(OrkPort recPort) {
		this.recPort = recPort;
	}

	public OrkUser getUser() {
		return user;
	}

	public void setUser(OrkUser user) {
		this.user = user;
	}
	
}
