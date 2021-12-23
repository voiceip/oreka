package net.sf.oreka.test;

import java.util.Collection;
import java.util.Iterator;

import net.sf.oreka.HibernateManager;
import net.sf.oreka.persistent.OrkLoginString;
import net.sf.oreka.persistent.OrkUser;

import org.apache.log4j.PropertyConfigurator;
import org.hibernate.Session;
import org.hibernate.Transaction;

public class FillDatabaseUser {

	static HibernateManager hibernateManager = HibernateManager.instance();	
	
	public static void main(String args[]) throws Exception
	{
		OrkUser usr = null;
		
		try {
			PropertyConfigurator.configure("c:/oreka/test/log4j.properties");
			hibernateManager.configure("c:/oreka/test/mysql.hbm.xml");

			
			Session hbnSession = hibernateManager.getSession();
			Transaction tx = hbnSession.beginTransaction();
			
			for(int i=0; i<100; i++) {
				OrkUser user = new OrkUser();
				user.setFirstname("fn" + i);
				user.setLastname("ln" + i);
				user.setPassword("password");
				hbnSession.save(user);
				
				OrkLoginString ls = new OrkLoginString();
				ls.setLoginString("ls" + i);
				//ls.bidirSetUser(user);
				ls.setUser(user);
				hbnSession.save(ls);
				
				OrkLoginString ls2 = new OrkLoginString();
				ls2.setLoginString("ls2" + i);
				//ls2.bidirSetUser(user);
				ls2.setUser(user);
				hbnSession.save(ls2);
				
			}
			tx.commit();
			hbnSession.close();
			
//			hbnSession = hibernateManager.getSession();
//			tx = hbnSession.beginTransaction();
//			usr = (User)hbnSession.get(User.class, 4);
//			tx.commit();
//			hbnSession.close();
			System.out.println("Done");
		}
		catch(Exception e) {
			e.printStackTrace();
		}
		
//		Collection col = usr.getLoginStrings();
//		String loginStringsCsv = "";
//		Iterator it = col.iterator();
//		while(it.hasNext()) {
//			loginStringsCsv = loginStringsCsv + ((LoginString)it.next()).getLoginString() + ":";
//		}
//		System.out.println("~~~" + usr.getFirstname() + " " + loginStringsCsv);
	}
}
