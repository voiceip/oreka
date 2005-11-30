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

/**
 * 
 */
package net.sf.oreka;

import java.io.File;

import net.sf.oreka.persistent.Domain;
import net.sf.oreka.persistent.LoginString;
import net.sf.oreka.persistent.RecPort;
import net.sf.oreka.persistent.RecPortFace;
import net.sf.oreka.persistent.RecProgram;
import net.sf.oreka.persistent.RecSegment;
import net.sf.oreka.persistent.RecSession;
import net.sf.oreka.persistent.RecTape;
import net.sf.oreka.persistent.Service;
import net.sf.oreka.persistent.User;

import org.apache.log4j.Level;
import org.apache.log4j.Logger;
import org.hibernate.HibernateException;
import org.hibernate.Session;
import org.hibernate.SessionFactory;
import org.hibernate.Transaction;
import org.hibernate.cfg.AnnotationConfiguration;


public class HibernateManager {
	
	private static SessionFactory sessionFactory = null;
	static Logger logger = Logger.getLogger(HibernateManager.class);
	
	public static void configure(String filename) throws Exception {
		
		File configFile = new File(filename);
		
		AnnotationConfiguration config = new AnnotationConfiguration();
		config.configure(configFile);
		
		config.addAnnotatedClass(RecProgram.class);
		config.addAnnotatedClass(RecSession.class);
		config.addAnnotatedClass(RecSegment.class);
		config.addAnnotatedClass(RecTape.class);
		config.addAnnotatedClass(User.class);
		config.addAnnotatedClass(LoginString.class);
		config.addAnnotatedClass(Domain.class);		
		config.addAnnotatedClass(Service.class);
		config.addAnnotatedClass(RecPort.class);
		config.addAnnotatedClass(RecPortFace.class);
		sessionFactory = config.buildSessionFactory();
		
		// Add admin user if does not exist yet
		Session hbnSession = null;
		Transaction tx = null;
		try
		{
			hbnSession = getSession();
			tx = hbnSession.beginTransaction();
			User admin = (User)hbnSession.get(User.class, 1);
			if(admin == null) {
				admin = new User();
				admin.setPassword("admin");
				LoginString ls = new LoginString();
				ls.setUser(admin);
				ls.setLoginString("admin");
				hbnSession.save(admin);
				hbnSession.save(ls);				
			}
			tx.commit();
		}
		catch ( HibernateException he ) {
			logger.error("Hibernate error:" + he.getClass().getName());
		}
		catch (Exception e)
		{
			logger.error("Exception:", e);
		}
		finally {
			if(hbnSession != null) {hbnSession.close();}
		}
	}
	
	public static Session getSession() throws Exception {
		if (sessionFactory == null) {
			throw new OrkException("HibernateManager.getSession: application must configure hibernate before using it.");
		}
		return sessionFactory.openSession();
	}
}
