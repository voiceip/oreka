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
import net.sf.oreka.persistent.RecPort;
import net.sf.oreka.persistent.RecPortFace;
import net.sf.oreka.persistent.RecSegment;
import net.sf.oreka.persistent.RecTape;
import net.sf.oreka.persistent.Service;
import net.sf.oreka.persistent.User;

import org.apache.log4j.Logger;
import org.hibernate.Session;

public class Port {

	public ArrayList<RecPortFace> portFaces = new ArrayList<RecPortFace>();
	private RecPort recPort = null;
	private TapeMessage lastTapeMessage = null;
	private MetadataMessage lastMetadataMessage = null;
	private User user = null;
	
	private LinkedList<RecSegment> recSegments = new LinkedList<RecSegment> ();
	private LinkedList<RecTape> recTapes = new LinkedList<RecTape> ();
	
	Logger log = null;
	
	public Port(RecPort recPort) {
		this.recPort = recPort;
		log = LogManager.getInstance().portLogger;
	}
	
	private synchronized void addTapeToLocalCache(RecTape tape) {
		
		if (recTapes.size() > 50) {
			recTapes.removeLast();
		}
		recTapes.addFirst(tape);
	}
	
	private synchronized void addSegmentToLocalCache(RecSegment seg) {
		
		if (recSegments.size() > 50) {
			recSegments.removeLast();
		}
		recSegments.addFirst(seg);
	}
	
	private boolean areTogether(RecTape tape, RecSegment seg) {
		
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
	
	private void joinTogether(RecTape tape, RecSegment seg, Session hbnSession) {

		seg.setRecTape(tape);
		seg.setRecTapeOffset(seg.getTimestamp().getTime() - tape.getTimestamp().getTime());
		hbnSession.save(seg);
	}
	
	private synchronized void findAndAssociateTape(RecSegment segment, Session hbnSession) {
		
		// Iterate over tapes and find the ones corresponding to this segment
		Iterator<RecTape> it = recTapes.iterator();
		boolean foundOne = false;
		boolean done = false;
		while (it.hasNext() && !done) {
			RecTape tape = it.next();
			if (areTogether(tape, segment)) {
				joinTogether(tape, segment, hbnSession);
				log.info("#" + recPort.getId() + ": associating segment:" + segment.getId() + " with tape:" + tape.getId());
				foundOne = true;
			}
			else {
				if (foundOne) {
					done = true;
				}
			}
		}
	}
	
	private synchronized void findAndAssociateSegment(RecTape tape, Session hbnSession) {
		
		// Iterate over segments and find the ones corresponding to this tape
		Iterator<RecSegment> it = recSegments.iterator();
		boolean foundOne = false;
		boolean done = false;
		while (it.hasNext() && !done) {
			RecSegment segment = it.next();
			if (areTogether(tape, segment)) {
				joinTogether(tape, segment, hbnSession);
				log.info("#" + recPort.getId() + ": associating segment:" + segment.getId() + " with tape:" + tape.getId());
				foundOne = true;
			}
			else {
				if (foundOne) {
					done = true;
				}
			}
		}
	}
	
	public void notifyMetadataMessage(MetadataMessage metadataMessage, Session hbnSession, Service srv) {
		
		long duration = 0;
		Date startDate = null;
		
		if (metadataMessage.getStage() == TapeMessage.CaptureStage.start) {
			lastMetadataMessage = metadataMessage;
		}
		else if (metadataMessage.getStage() == TapeMessage.CaptureStage.stop) {
			
			if (lastMetadataMessage != null) {
				duration = ((long)metadataMessage.getTimestamp() - (long)lastMetadataMessage.getTimestamp())*1000;
				startDate = new Date((long)lastMetadataMessage.getTimestamp()*1000);
			}
		}
		else if (metadataMessage.getStage() == TapeMessage.CaptureStage.complete) {
			duration = (long)metadataMessage.getDuration()*1000;
			startDate = new Date(metadataMessage.getTimestamp()*1000);
		}
		
		if (startDate != null) {
			
			if (srv.isRecordMaster()) {
				// create segment
				RecSegment recSegment = new RecSegment();
				recSegment.setTimestamp(startDate);
				recSegment.setDirection(metadataMessage.getDirection());
				recSegment.setDuration(duration);
				recSegment.setLocalParty(metadataMessage.getLocalParty());
				recSegment.setLocalEntryPoint(metadataMessage.getLocalEntryPoint());
				recSegment.setRecSessionOffset(0);
				recSegment.setPort(recPort);
				
				if(metadataMessage.getLocalParty() != "") {
					User user = UserManager.instance().getByLoginString(metadataMessage.getLocalParty(), hbnSession);
					recSegment.setUser(user);
				}
				
				// is this retained by a program ?
				if (ProgramManager.instance().filterSegmentAgainstAllPrograms(recSegment, hbnSession)) {
					// Try to find associated tape and link it to this
					addSegmentToLocalCache(recSegment);
					findAndAssociateTape(recSegment, hbnSession);
					hbnSession.save(recSegment);
					log.info("#" + metadataMessage.getCapturePort() + ": written segment " + recSegment.getId());
				}
				else {
					log.info("#" + metadataMessage.getCapturePort() + ":  metadata message discarded");
				}
			}
		}
	}
	
	public void notifyTapeMessage(TapeMessage tapeMessage, Session hbnSession, Service srv) {
		
		if (tapeMessage.getStage() == TapeMessage.CaptureStage.start) {
			lastTapeMessage = tapeMessage;
		}
		else {
			// Tape stop
			if (lastTapeMessage == null) {
				log.warn("Port: notifyTapeMessage: stop without a previous tape message");
			}
			else if (lastTapeMessage.getStage() != TapeMessage.CaptureStage.start) {
				log.warn("Port: notifyTapeMessage: stop without a start");	
			}
			else if (tapeMessage.getStage() == TapeMessage.CaptureStage.stop){
				boolean generateSegment = false;
				if (portFaces.size() == 1 || srv.isRecordMaster()) {
					generateSegment = true;
				}
				
				TapeMessage startMessage = lastTapeMessage;
				TapeMessage stopMessage = tapeMessage;
				long duration = (stopMessage.getTimestamp() - startMessage.getTimestamp())*1000;
				long date = ((long)startMessage.getTimestamp()) * 1000;
				Date timestamp = new Date(date);
	            
				// create a new tape record
				RecTape recTape = new RecTape();
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
				log.info("#" + tapeMessage.getCapturePort() + ": written tape " + recTape.getId());
				
				if (generateSegment) {
					RecSegment recSegment = new RecSegment();
					recSegment.setTimestamp(timestamp);
					recSegment.setDirection(stopMessage.getDirection());
					recSegment.setDuration(duration);
					recSegment.setLocalParty(stopMessage.getLocalParty());
					recSegment.setLocalEntryPoint(stopMessage.getLocalEntryPoint());
					recSegment.setRecTape(recTape);
					recSegment.setPort(recPort);
					
					if(stopMessage.getLocalParty() != "") {
						User user = UserManager.instance().getByLoginString(stopMessage.getLocalParty(), hbnSession);
						recSegment.setUser(user);
					}
					if (ProgramManager.instance().filterSegmentAgainstAllPrograms(recSegment, hbnSession)) {
						hbnSession.save(recSegment);
						log.info("#" + tapeMessage.getCapturePort() + ": written segment " + recSegment.getId());
					}
					else {
						log.info("#" + tapeMessage.getCapturePort() + ":  tape message discarded");					}
				}
				else {
					// segments are generated by metadata messages
					addTapeToLocalCache(recTape);
					findAndAssociateSegment(recTape, hbnSession);
				}
			}
		}
	}

	public RecPort getRecPort() {
		return recPort;
	}

	public void setRecPort(RecPort recPort) {
		this.recPort = recPort;
	}

	public User getUser() {
		return user;
	}

	public void setUser(User user) {
		this.user = user;
	}
	
}
