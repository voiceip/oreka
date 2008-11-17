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

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Iterator;

import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestSuite;
import net.sf.oreka.Cycle;
import net.sf.oreka.Day;
import net.sf.oreka.Direction;
import net.sf.oreka.HibernateManager;
import net.sf.oreka.orktrack.OrkTrack;
import net.sf.oreka.orktrack.ProgramManager;
import net.sf.oreka.persistent.OrkLoginString;
import net.sf.oreka.persistent.OrkProgram;
import net.sf.oreka.persistent.OrkSegment;
import net.sf.oreka.persistent.OrkUser;

import org.hibernate.Session;
import org.hibernate.Transaction;

public class ProgramTest extends TestCase {
	
	public static void main (String[] args) {
		junit.textui.TestRunner.run (suite());
	}
	protected void setUp() throws Exception {
		OrkTrack.initialize("C:/oreka/orktrack/log4j.properties", "C:/oreka/orktrack/hsqldb.hbm.xml", "C:/oreka/orktrack/orktrack.config.xml");
	}
	public static Test suite() {
		return new TestSuite(ProgramTest.class);
	}
	
	public void test1() throws Exception {
		

		Session hbnSession = OrkTrack.hibernateManager.getSession();
		Transaction tx = hbnSession.beginTransaction();
	
		// create a user
		OrkUser user = new OrkUser();
		user.setFirstname("mawagade");
		hbnSession.save(user);
		OrkLoginString ls = new OrkLoginString();
		ls.setLoginString("1234");
		ls.setUser(user);
		hbnSession.save(ls);
		
		// create a program
		OrkProgram prog1 = new OrkProgram();
		prog1.setDirection(Direction.IN);
		prog1.setTargetUser(user);
		hbnSession.save(prog1);
		
		tx.commit();
		hbnSession.close();
		
		ProgramManager.instance().load();
		
		// segment 1: should pass
		OrkSegment seg = new OrkSegment();
		seg.setDirection(Direction.IN);
		seg.setUser(user);
		
		hbnSession = OrkTrack.hibernateManager.getSession();
		tx = hbnSession.beginTransaction();
		if (ProgramManager.instance().filterSegmentAgainstAllPrograms(seg, hbnSession)) {
			hbnSession.save(seg);
		}
		tx.commit();
		hbnSession.close();
		// verify result

		hbnSession = OrkTrack.hibernateManager.getSession();
		tx = hbnSession.beginTransaction();
		OrkProgram prog = (OrkProgram)hbnSession.load(OrkProgram.class, prog1.getId());
		assertTrue(prog.getRecordedSoFar() == 1);
		Iterator<OrkSegment> it = prog.getRecSegments().iterator();
		assertTrue(it.next().getId() == seg.getId());
		tx.commit();
		hbnSession.close();
	}
	
	public void testSchedule() throws Exception {
		
		// create a "daily" program
		OrkProgram progDaily = new OrkProgram();
		progDaily.setDirection(Direction.ALL);
		progDaily.setCycle(Cycle.DAILY);
		progDaily.setStartTime(new Date(10*3600*1000));	// 10:00 am
		progDaily.setStopTime(new Date(11*3600*1000));  // 11:00 am
		
		// create a "weekly" program
		OrkProgram progWeekly = new OrkProgram();
		progWeekly.setDirection(Direction.ALL);
		progWeekly.setCycle(Cycle.WEEKLY);
		progWeekly.setStartDay(Day.TUESDAY);
		progWeekly.setStopDay(Day.THURSDAY);		
		progWeekly.setStartTime(new Date(22*3600*1000));	// 10:00 pm
		progWeekly.setStopTime(new Date(23*3600*1000));  // 11:00 pm


		// segment 1: should pass
		OrkSegment seg = new OrkSegment();
		SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
		Date segTimestamp = sdf.parse("2005-11-29 10:30:00");
		seg.setTimestamp(segTimestamp);  // 10:30
		assertTrue(ProgramManager.instance().filterSegmentAgainstProgram(seg, progDaily, null));
		
		// segment 2: should be rejected
		seg.setTimestamp(sdf.parse("2005-11-29 11:00:01"));
		assertFalse(ProgramManager.instance().filterSegmentAgainstProgram(seg, progDaily, null));

		// segment 3: should pass
		seg.setTimestamp(sdf.parse("2005-11-29 22:45:01")); // tuesday
		//int dateDay = seg.getTimestamp().getDay();
		//GregorianCalendar cal = new GregorianCalendar();
		//cal.setTimeInMillis(seg.getTimestamp().getTime());
		//int calDay = cal.get(Calendar.DAY_OF_WEEK);
		//int todayDay = new Date().getDay();
		assertTrue(ProgramManager.instance().filterSegmentAgainstProgram(seg, progWeekly, null));
		
		// segment 4: should pass
		seg.setTimestamp(sdf.parse("2005-12-01 22:45:01")); // thursday 
		assertTrue(ProgramManager.instance().filterSegmentAgainstProgram(seg, progWeekly, null));
	
		// segment 5: should be rejected
		seg.setTimestamp(sdf.parse("2005-11-27 22:45:01")); // sunday 
		assertFalse(ProgramManager.instance().filterSegmentAgainstProgram(seg, progWeekly, null));
	}
	
//	public void testManyToMany() throws Exception {
//		
//		Session hbnSession = HibernateManager.getSession();
//		Transaction tx = hbnSession.beginTransaction();
//		
//		// Create two programs
//		RecProgram prog1 = new RecProgram();
//		hbnSession.save(prog1);
//		RecProgram prog2 = new RecProgram();
//		hbnSession.save(prog2);
//		
//		// create 1000 segments
//		for(int i=0; i<1000; i++) {
//			RecSegment seg = new RecSegment();
//			seg.getRecPrograms().add(prog1);
//			seg.getRecPrograms().add(prog2);
//			hbnSession.save(seg);
//		}
//		
//		tx.commit();
//		hbnSession.close();
//		
//		hbnSession = HibernateManager.getSession();
//		tx = hbnSession.beginTransaction();
//		
//		// create a new segment and add it to the program
//		RecProgram prog = (RecProgram)hbnSession.load(RecProgram.class, prog2.getId());
//		RecSegment seg = new RecSegment();
//		seg.getRecPrograms().add(prog);
//		hbnSession.save(seg);
//		
//		tx.commit();
//		hbnSession.close();
//	}
}
