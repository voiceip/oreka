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
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Properties;

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

import org.apache.log4j.Logger;
import org.hibernate.HibernateException;
import org.hibernate.Session;
import org.hibernate.SessionFactory;
import org.hibernate.Transaction;
import org.hibernate.cfg.AnnotationConfiguration;
import org.logicalcobwebs.proxool.ProxoolFacade;


public class HibernateManager {
	
	private static HibernateManager hibernateManager = null;
	private SessionFactory sessionFactory = null;
	static Logger logger = Logger.getLogger(HibernateManager.class);
	
	private HibernateManager() {
	}
	
	public static HibernateManager instance() {
		if(hibernateManager == null) {
			hibernateManager = new HibernateManager();
		}
		return hibernateManager;
	}
	
	public void configure(String filename) throws Exception {
		
		File configFile = new File(filename);
		
		AnnotationConfiguration config = new AnnotationConfiguration();
		config.configure(configFile);
		
		// Configure the proxool connection pool
		Class.forName("org.logicalcobwebs.proxool.ProxoolDriver");
		Properties info = new Properties();
		info.setProperty("proxool.maximum-connection-count", "10");
		info.setProperty("proxool.house-keeping-test-sql", "select CURRENT_DATE");
		info.setProperty("user", config.getProperty("hibernate.connection.username"));
		info.setProperty("password", config.getProperty("hibernate.connection.password"));
		SimpleDateFormat sdf = new SimpleDateFormat("HHmmss");
		// Each time a pool is configured, it will have a different alias.
		// This is so that we don't get a "pool already registered" error.
		String alias = "oreka" + sdf.format(new Date());
		String driverClass = config.getProperty("hibernate.connection.driver_class");
		String driverUrl = config.getProperty("hibernate.connection.url");
		String url = "proxool." + alias + ":" + driverClass + ":" + driverUrl;
		ProxoolFacade.registerConnectionPool(url, info);
		
		// Let hibernate know we want to use proxool
		config.setProperty("hibernate.proxool.pool_alias", alias);
		config.setProperty("hibernate.proxool.existing_pool", "true");
		
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
	
	public Session getSession() throws Exception {
		if (sessionFactory == null) {
			throw new OrkException("HibernateManager.getSession: application must configure hibernate before using it.");
		}
		return sessionFactory.openSession();
	}
}
