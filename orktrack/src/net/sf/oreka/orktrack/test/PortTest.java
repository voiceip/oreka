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

package net.sf.oreka.orktrack.test;

import java.util.Date;
import java.util.GregorianCalendar;
import java.util.Iterator;

import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestSuite;
import net.sf.oreka.Direction;
import net.sf.oreka.HibernateManager;
import net.sf.oreka.orktrack.OrkTrack;
import net.sf.oreka.orktrack.PortManager;
import net.sf.oreka.orktrack.ProgramManager;
import net.sf.oreka.orktrack.ServiceManager;
import net.sf.oreka.orktrack.messages.MetadataMessage;
import net.sf.oreka.orktrack.messages.TapeMessage;
import net.sf.oreka.persistent.OrkLoginString;
import net.sf.oreka.persistent.OrkProgram;
import net.sf.oreka.persistent.OrkSegment;
import net.sf.oreka.persistent.OrkTape;
import net.sf.oreka.persistent.OrkService;
import net.sf.oreka.persistent.OrkUser;

import org.hibernate.Session;
import org.hibernate.Transaction;

public class PortTest extends TestCase {
	
	public static void main (String[] args) {
		junit.textui.TestRunner.run (suite());
	}
	protected void setUp() throws Exception {
		OrkTrack.initialize("C:/oreka/orktrack/log4j.properties", "C:/oreka/orktrack/hsqldb.hbm.xml", "C:/oreka/orktrack/orktrack.config.xml");
	}
	public static Test suite() {
		return new TestSuite(PortTest.class);
	}

//	=== commented out because touches private elements	
//	public void testAreTogether() {
//		
//		Port port = new Port();
//		
//		long now = new Date().getTime();
//		RecTape tape = new RecTape();
//		tape.setTimestamp(new Date(now));
//		tape.setDuration(10000);
//		
//		RecSegment seg = new RecSegment();
//		seg.setTimestamp(new Date(now));
//		seg.setDuration(10000);
//		
//		assertTrue(port.areTogether(tape, seg));
//		
//		tape.setTimestamp(new Date(now-2000));
//		assertTrue(port.areTogether(tape, seg));
//		
//		tape.setTimestamp(new Date(now-4000));
//		assertFalse(port.areTogether(tape, seg));
//		
//		tape.setTimestamp(new Date(now));
//		seg.setDuration(12000);
//		assertTrue(port.areTogether(tape, seg));
//		
//		seg.setDuration(14000);
//		assertFalse(port.areTogether(tape, seg));
//		
//		tape.setTimestamp(new Date(now));
//		tape.setDuration(10000);
//		seg.setTimestamp(new Date(now+5000));
//		seg.setDuration(5000);
//		assertTrue(port.areTogether(tape, seg));
//		
//		seg.setDuration(8000);
//		assertFalse(port.areTogether(tape, seg));
//		
//	}
	
//=== commented out because touches private elements	
//	public void testFindAndAssociateTape()  throws Exception {
//		
//		Session hbnSession = HibernateManager.getSession();
//		Transaction tx = hbnSession.beginTransaction();
//		
//		// case 1, a segment arrives after a tape
//		Port port = new Port();
//		RecTape tape1 = new RecTape();		// older tape
//		RecTape tape2 = new RecTape();		// newer tape
//		RecTape fooTape = new RecTape();	// this is the one
//		long now = new Date().getTime();
//		fooTape.setTimestamp(new Date(now));
//		tape1.setTimestamp(new Date(now-10000));
//		tape2.setTimestamp(new Date(now+10000));
//		fooTape.setDuration(10000);
//		tape1.setDuration(10000);
//		tape2.setDuration(10000);
//		
//		port.recTapes.addFirst(tape1);
//		port.recTapes.addFirst(fooTape);
//		port.recTapes.addFirst(tape2);
//
//		RecSegment seg = new RecSegment();
//		seg.setTimestamp(new Date(now));
//		seg.setDuration(5000);
//		port.findAndAssociateTape(seg, hbnSession);
//		
//		assertTrue(seg.getRecTape() == fooTape);
//		assertTrue(seg.getRecTapeOffset() == 0);
//		
//		// case 2, a second segment arrives and is part of the same tape
//		RecSegment seg2 = new RecSegment();
//		seg2.setTimestamp(new Date(now+4000));
//		seg2.setDuration(5000);
//		port.findAndAssociateTape(seg2, hbnSession);
//		
//		assertTrue(seg2.getRecTape() == fooTape);
//		assertTrue(seg2.getRecTapeOffset() == 4000);
//		
//		hbnSession.close();
//	}
	
//	public void testTapeMessage() throws Exception {	
//		
//		// Generate start and stop messages
//		TapeMessage startMsg = new TapeMessage();
//		startMsg.setCapturePort("port1");
//		startMsg.setService("service1");
//		startMsg.setStage(TapeMessage.CaptureStage.start);
//		long startTime = new Date().getTime();
//		int startTimestamp = (int)(startTime/1000);
//		startMsg.setTimestamp(startTimestamp);
//		
//		startMsg.process();
//		
//		TapeMessage stopMsg = new TapeMessage();
//		stopMsg.setCapturePort("port1");
//		stopMsg.setDirection(Direction.in);
//		stopMsg.setFilename("test.wav");
//		stopMsg.setLocalEntryPoint("dnis1");
//		stopMsg.setLocalParty("9833");
//		stopMsg.setRemoteParty("514-425-5678");
//		stopMsg.setService("service1");
//		stopMsg.setStage(TapeMessage.CaptureStage.stop);
//		stopMsg.setTimestamp(startTimestamp + 10);
//		
//		stopMsg.process();
//		
//		// verify database entities.
//		Session hbnSession = HibernateManager.getSession();
//		Transaction tx = hbnSession.beginTransaction();
//		long time = ((long)startTimestamp)*1000;
//		GregorianCalendar cal = new GregorianCalendar();
//		cal.setTimeInMillis(time);
//			
//		RecSegment seg = null;
//		Iterator segments = hbnSession.createQuery(
//	    	"from RecSegment as seg where seg.timestamp=:date")
//	    	.setCalendar("date", cal)
//	    	.list()
//	    	.iterator();
//		if(segments.hasNext()) {
//			seg = (RecSegment)segments.next();
//			assertTrue(seg.getLocalParty().equals("9833"));
//			
//			RecTape tape = seg.getRecTape();
//			assertTrue(tape.getFilename().equals("test.wav"));
//		}
//		else {
//			fail();
//		}
//		tx.commit();
//		hbnSession.close();
//	}
	
	public void testTapeAndMetadataMessage() throws Exception {
		
		Session hbnSession = OrkTrack.hibernateManager.getSession();
		Transaction tx = hbnSession.beginTransaction();
		OrkService recService = ServiceManager.retrieveOrCreate("recservice", "localhost", hbnSession);
		OrkService ctiService = ServiceManager.retrieveOrCreate("ctiservice", "localhost", hbnSession);		
		ctiService.setRecordMaster(true);
		hbnSession.save(recService);
		
		OrkUser user = new OrkUser();
		user.setFirstname("salisse");
		OrkLoginString ls = new OrkLoginString();
		ls.setUser(user);
		ls.setLoginString("1973");
		hbnSession.save(user);
		hbnSession.save(ls);
		
		PortManager.instance().addPort("recport", "ctiport", hbnSession);
		
		// create program that does not filter anything
		OrkProgram prog1 = new OrkProgram();
		hbnSession.save(prog1);
		ProgramManager.instance().addProgram(prog1);
		
		tx.commit();
		hbnSession.close();
		

		
		// Generate tape start and stop messages
		TapeMessage startMsg = new TapeMessage();
		startMsg.setCapturePort("recport");
		startMsg.setService("recservice");
		startMsg.setStage(TapeMessage.CaptureStage.START);
		long startTime = new Date().getTime();
		int startTimestamp = (int)(startTime/1000);
		startMsg.setTimestamp(startTimestamp);
		
		startMsg.process();
		
		TapeMessage stopMsg = new TapeMessage();
		stopMsg.setCapturePort("recport");
		stopMsg.setService("recservice");
		stopMsg.setDirection(Direction.IN);
		stopMsg.setFilename("test.wav");
		stopMsg.setLocalEntryPoint("dnis1");
		stopMsg.setLocalParty("9833");
		stopMsg.setRemoteParty("514-425-5678");
		stopMsg.setStage(TapeMessage.CaptureStage.STOP);
		stopMsg.setTimestamp(startTimestamp + 10);
		
		stopMsg.process();
		
		// Generate metadata start and stop messages
		MetadataMessage mdStartMsg = new MetadataMessage();
		mdStartMsg.setStage(TapeMessage.CaptureStage.START);
		mdStartMsg.setTimestamp(startTimestamp + 3);
		mdStartMsg.setCapturePort("ctiport");
		mdStartMsg.setService("ctiservice");
		
		mdStartMsg.process();
		
		MetadataMessage mdStopMsg = new MetadataMessage();
		mdStopMsg.setStage(TapeMessage.CaptureStage.STOP);
		mdStopMsg.setLocalParty("1973");
		mdStopMsg.setTimestamp(startTimestamp + 5);
		mdStopMsg.setCapturePort("ctiport");
		mdStopMsg.setService("ctiservice");
		
		mdStopMsg.process();
		
		// verify database entities.
		hbnSession = OrkTrack.hibernateManager.getSession();
		tx = hbnSession.beginTransaction();
		long time = ((long)(startTimestamp+3))*1000;
		GregorianCalendar cal = new GregorianCalendar();
		cal.setTimeInMillis(time);
			
		OrkSegment seg = null;
		Iterator segments = hbnSession.createQuery(
	    	"from RecSegment as seg where seg.timestamp=:date")
	    	.setCalendar("date", cal)
	    	.list()
	    	.iterator();
		if(segments.hasNext()) {
			seg = (OrkSegment)segments.next();
			assertTrue(seg.getLocalParty().equals("1973"));
			assertTrue(seg.getTapeOffset() == 3000);
			
			OrkTape tape = seg.getTape();
			assertTrue(tape.getFilename().equals("test.wav"));
			
			OrkUser user2 = seg.getUser();
			assertTrue(user.getId() == user2.getId());
		}
		else {
			fail();
		}
		tx.commit();
		hbnSession.close();
	}
	
//	public void testUserManager() throws Exception {
//		
//		Session hbnSession = HibernateManager.getSession();
//		Transaction tx = hbnSession.beginTransaction();
//		User user = new User();
//		user.setFirstname("salisse");
//		LoginString ls = new LoginString();
//		ls.setUser(user);
//		ls.setLoginString("4568");
//		hbnSession.save(user);
//		hbnSession.save(ls);
//		User user2 = UserManager.getByLoginString("4568", hbnSession);
//		assertTrue(user.getFirstname().equals(user2.getFirstname()));
//		tx.commit();
//		hbnSession.close();
//	}
}
