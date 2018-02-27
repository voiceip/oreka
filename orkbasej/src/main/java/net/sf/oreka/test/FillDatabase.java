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
import net.sf.oreka.persistent.OrkLoginString;
import net.sf.oreka.persistent.OrkProgram;
import net.sf.oreka.persistent.OrkSegment;
import net.sf.oreka.persistent.OrkTape;
import net.sf.oreka.persistent.OrkService;
import net.sf.oreka.persistent.OrkUser;

import org.hibernate.Session;
import org.hibernate.Transaction;

public class FillDatabase {

	static HibernateManager hibernateManager = HibernateManager.instance();
	
	public static void main(String args[]) throws Exception
	{
		hibernateManager.configure("c:/oreka/test/mysql.hbm.xml");
		
		Session hbnSession = hibernateManager.getSession();
		Transaction tx = hbnSession.beginTransaction();
		
		OrkService service = new OrkService();
		service.setFileServePath("");
		service.setFileServeProtocol("http");
		service.setFileServeTcpPort(8080);
		service.setHostname("localhost");
		service.setRecordMaster(true);
		service.setName("orkaudio1");
		hbnSession.save(service);
		
		// user 1
		OrkUser user = new OrkUser();
		user.setFirstname("Raymond");
		user.setLastname("Stein");
		user.setPassword("raymond");
		OrkLoginString ls = new OrkLoginString();
		ls.setUser(user);
		ls.setLoginString("2005");
		OrkLoginString lsA = new OrkLoginString();
		lsA.setUser(user);
		lsA.setLoginString("2006");
		OrkLoginString lsB = new OrkLoginString();
		lsB.setUser(user);
		lsB.setLoginString("raymond");
		hbnSession.save(user);
		hbnSession.save(ls);
		hbnSession.save(lsA);
		hbnSession.save(lsB);
		
		// user 2
		OrkUser user2 = new OrkUser();
		user2.setFirstname("Bert");
		user2.setLastname("Nolte");		
		user2.setPassword("bert");
		OrkLoginString ls2 = new OrkLoginString();
		ls2.setUser(user2);
		ls2.setLoginString("2000");
		OrkLoginString ls2A = new OrkLoginString();
		ls2A.setUser(user2);
		ls2A.setLoginString("2001");
		OrkLoginString ls2B = new OrkLoginString();
		ls2B.setUser(user2);
		ls2B.setLoginString("bert");
		hbnSession.save(user2);
		hbnSession.save(ls2);
		hbnSession.save(ls2A);
		hbnSession.save(ls2B);
		
		// create program that does not filter anything
		OrkProgram prog1 = new OrkProgram();
		prog1.setName("Test program");
		hbnSession.save(prog1);
		
		// Create a bunch of segments
		String lastParty = "123-234-5678";
		java.util.Random generator = new java.util.Random();
		for(int i=0; i<500; i++) {
			OrkSegment seg = new OrkSegment();
			OrkTape tape = new OrkTape();
			
			tape.setService(service);
			tape.setFilename("f1.mp3");
			
			Date date = new Date(new Date().getTime() - 600000 +(i*1000));
			seg.setTimestamp(date);
			tape.setTimestamp(date);
			
			String randomString = "" + generator.nextInt();
			randomString = randomString.replace("-", "a");
			seg.setRemoteParty(lastParty);
			lastParty = randomString;
			
			float randomDirection = generator.nextFloat();
			
			if(randomDirection > .8){
				if(randomDirection > .95) {
					seg.setLocalParty(ls.getLoginString());
				}
				else {
					seg.setLocalParty(lsA.getLoginString());					
				}
				seg.setUser(user);
			}
			else {
				if(randomDirection > .3) {
					seg.setLocalParty(ls2.getLoginString());
				}
				else {
					seg.setLocalParty(ls2A.getLoginString());
				}
				seg.setUser(user2);
			}
			
			randomDirection = generator.nextFloat();
			if(randomDirection > .5){
				seg.setDirection(Direction.IN);
			}
			else {
				seg.setDirection(Direction.OUT);				
			}
			
			long duration = (long)(generator.nextFloat()*60.0);
			seg.setDuration(duration);
			tape.setDuration(duration+1);
			
			seg.setTape(tape);
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
