package net.sf.oreka.srvc.test;

import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestSuite;
import net.sf.oreka.HibernateManager;
import net.sf.oreka.persistent.OrkLoginString;
import net.sf.oreka.persistent.OrkUser;
import net.sf.oreka.srvc.ObjectService;
import net.sf.oreka.srvc.ObjectServiceHbn;
import net.sf.oreka.srvc.UserService;
import net.sf.oreka.srvc.UserServiceHbn;

import org.apache.log4j.PropertyConfigurator;
import org.hibernate.Session;
import org.hibernate.Transaction;

public class UserServiceHbnTest extends TestCase {

	static HibernateManager hibernateManager = HibernateManager.instance();
	static UserService usrv = new UserServiceHbn();
	static ObjectService osrv = new ObjectServiceHbn();
	
	public static void main (String[] args) throws Exception {
		
		PropertyConfigurator.configure("c:/oreka/test/log4j.properties");
		hibernateManager.configure("c:/oreka/test/mysql.hbm.xml");
		
		junit.textui.TestRunner.run (suite());
	}
	
	public static Test suite() {
		return new TestSuite(UserServiceHbnTest.class);
	}
	
//	public void printOutUser(int userId) {
//		User user = (User)osrv.getObjectById(User.class, userId);
//		System.out.println("\n User:" + user.getFirstname());
//		for(LoginString ls : user.getLoginStrings()){
//			System.out.println(ls.getLoginString());
//		}
//	}
	
	public void testSetUserLoginStrings() throws Exception {
		
		
		Session hbnSession = hibernateManager.getSession();
		Transaction tx = hbnSession.beginTransaction();
		OrkUser user1 = new OrkUser();
		user1.setFirstname("fn1");
		hbnSession.save(user1);
		
		OrkUser user2 = new OrkUser();
		user2.setFirstname("fn2");
		hbnSession.save(user2);
		
		OrkUser deletedUser = new OrkUser();
		deletedUser.setFirstname("dlt");
		//deletedUser.setDeleted(true);
		hbnSession.save(deletedUser);
		
		tx.commit();
		hbnSession.close();
		
		/*
		usrv.setUserLoginStrings(user1.getId(), "ls1, ls2 ls3");
		printOutUser(user1.getId());
		
		usrv.setUserLoginStrings(user2.getId(), "ls4, ls5");
		printOutUser(user2.getId());
		
		usrv.setUserLoginStrings(deletedUser.getId(), "ls6");
		printOutUser(deletedUser.getId());
		
		System.out.println("** user1 looses ls1 **");
		usrv.setUserLoginStrings(user1.getId(), "ls2 ls3");
		printOutUser(user1.getId());
		
		usrv.deleteUser(deletedUser.getId());
		
		System.out.println("** user2 gets ls6 and looses ls4 and ls5 **");
		usrv.setUserLoginStrings(user2.getId(), "ls6");
		printOutUser(user2.getId());
		
		System.out.println("** user1 fails to get ls6 **");
		usrv.setUserLoginStrings(user1.getId(), "ls6");
		printOutUser(user1.getId());
		*/
		
	}
}
