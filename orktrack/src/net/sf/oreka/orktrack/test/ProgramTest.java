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

import java.util.Iterator;

import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestSuite;
import net.sf.oreka.Direction;
import net.sf.oreka.HibernateManager;
import net.sf.oreka.orktrack.OrkTrack;
import net.sf.oreka.orktrack.ProgramManager;
import net.sf.oreka.persistent.LoginString;
import net.sf.oreka.persistent.RecProgram;
import net.sf.oreka.persistent.RecSegment;
import net.sf.oreka.persistent.User;

import org.hibernate.Session;
import org.hibernate.Transaction;

public class ProgramTest extends TestCase {
	
	public static void main (String[] args) {
		junit.textui.TestRunner.run (suite());
	}
	protected void setUp() throws Exception {
		OrkTrack.initialize("C:/oreka/log4j.properties", "C:/oreka/mysql.hbm.xml", "C:/oreka/orktrack.config.xml");
	}
	public static Test suite() {
		return new TestSuite(ProgramTest.class);
	}
	
	public void test1() throws Exception {
		

		Session hbnSession = HibernateManager.getSession();
		Transaction tx = hbnSession.beginTransaction();
	
		// create a user
		User user = new User();
		user.setFirstname("mawagade");
		hbnSession.save(user);
		LoginString ls = new LoginString();
		ls.setLoginString("1234");
		ls.setUser(user);
		hbnSession.save(ls);
		
		// create a program
		RecProgram prog1 = new RecProgram();
		prog1.setDirection(Direction.IN);
		prog1.setTargetUser(user);
		hbnSession.save(prog1);
		
		tx.commit();
		hbnSession.close();
		
		ProgramManager.instance().load();
		
		RecSegment seg = new RecSegment();
		seg.setDirection(Direction.IN);
		seg.setUser(user);
		
		hbnSession = HibernateManager.getSession();
		tx = hbnSession.beginTransaction();
		if (ProgramManager.instance().filterSegmentAgainstAllPrograms(seg, hbnSession)) {
			hbnSession.save(seg);
		}
		tx.commit();
		hbnSession.close();
		
		// verify result
		hbnSession = HibernateManager.getSession();
		tx = hbnSession.beginTransaction();
		RecProgram prog = (RecProgram)hbnSession.load(RecProgram.class, prog1.getId());
		assertTrue(prog.getRecordedSoFar() == 1);
		Iterator<RecSegment> it = prog.getRecSegments().iterator();
		assertTrue(it.next().getId() == seg.getId());
		tx.commit();
		hbnSession.close();
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
