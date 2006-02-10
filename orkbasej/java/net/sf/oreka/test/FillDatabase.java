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

package net.sf.oreka.test;

import java.util.Date;

import net.sf.oreka.Direction;
import net.sf.oreka.HibernateManager;
import net.sf.oreka.persistent.LoginString;
import net.sf.oreka.persistent.RecProgram;
import net.sf.oreka.persistent.RecSegment;
import net.sf.oreka.persistent.RecTape;
import net.sf.oreka.persistent.Service;
import net.sf.oreka.persistent.User;

import org.hibernate.Session;
import org.hibernate.Transaction;

public class FillDatabase {

	static HibernateManager hibernateManager = HibernateManager.instance();
	
	public static void main(String args[]) throws Exception
	{
		hibernateManager.configure("c:/oreka/test/mysql.hbm.xml");
		
		Session hbnSession = hibernateManager.getSession();
		Transaction tx = hbnSession.beginTransaction();
		
		Service service = new Service();
		service.setFileServePath("");
		service.setFileServeProtocol("http");
		service.setFileServeTcpPort(8080);
		service.setHostname("localhost");
		service.setRecordMaster(true);
		service.setName("orkaudio1");
		hbnSession.save(service);
		
		// user 1
		User user = new User();
		user.setFirstname("salisse");
		user.setPassword("salisse");
		LoginString ls = new LoginString();
		ls.setUser(user);
		ls.setLoginString("1973");
		hbnSession.save(user);
		hbnSession.save(ls);
		
		// user 2
		User user2 = new User();
		user2.setFirstname("zapata");
		user2.setPassword("zapata");
		LoginString ls2 = new LoginString();
		ls2.setUser(user2);
		ls2.setLoginString("1974");
		hbnSession.save(user2);
		hbnSession.save(ls2);
		
		// create program that does not filter anything
		RecProgram prog1 = new RecProgram();
		prog1.setName("Test program");
		hbnSession.save(prog1);
		
		// Create a bunch of segments
		String lastParty = "123-234-5678";
		java.util.Random generator = new java.util.Random();
		for(int i=0; i<500; i++) {
			RecSegment seg = new RecSegment();
			RecTape tape = new RecTape();
			
			tape.setService(service);
			tape.setFilename("f1.mp3");
			
			Date date = new Date(new Date().getTime() - 600000 +(i*1000));
			seg.setTimestamp(date);
			tape.setTimestamp(date);
			
			String randomString = "" + generator.nextInt();
			randomString = randomString.replace("-", "a");
			seg.setLocalParty(randomString);
			seg.setUser(user);
			seg.setRemoteParty(lastParty);
			lastParty = randomString;
			
			float randomDirection = generator.nextFloat();
			if(randomDirection > .5){
				seg.setDirection(Direction.IN);
			}
			else {
				seg.setDirection(Direction.OUT);				
			}
			
			long duration = (long)(generator.nextFloat()*60.0);
			seg.setDuration(duration);
			tape.setDuration(duration+1);
			
			seg.setRecTape(tape);
			hbnSession.save(tape);
			hbnSession.save(seg);
		}
		
//		StringBuffer queryString = new StringBuffer("from RecSegment as seg ");
//		Query query = hbnSession.createQuery(queryString.toString());
//		
//		ScrollableResults scrollDocs = query.scroll();		
//		scrollDocs.first();
//		
//		while (scrollDocs.get()!= null)
//		{
//			RecSegment seg = (RecSegment)scrollDocs.get(0);
//			//RecTape tape = (RecTape)scrollDocs.get(1);
//			//RecTape tape = new RecTape();
//			System.out.println(seg.getId());
//			scrollDocs.next();
//		}
		
		tx.commit();
		hbnSession.close();
		
		System.out.println("Done");
	}
	
}
