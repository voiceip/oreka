package net.sf.oreka.services;

import java.util.ArrayList;

import net.sf.oreka.HibernateManager;
import net.sf.oreka.orkweb.ContextListener;
import net.sf.oreka.orkweb.OrkWeb;
import net.sf.oreka.persistent.OrkUser;

import org.apache.log4j.Level;
import org.apache.log4j.Logger;
import org.hibernate.HibernateException;
import org.hibernate.Query;
import org.hibernate.Session;
import org.hibernate.Transaction;

public class UserServiceHbn implements UserService {

	static Logger logger = Logger.getLogger(UserServiceHbn.class);	
	
	public OrkUser login(String username, String password) {
		
		Session hbnSession = null;
		OrkUser user = null;
		
		logger.debug("Trying to login user:" + username + " with passwd:" + password);
		
		try
		{
			hbnSession = OrkWeb.hibernateManager.getSession();
			
			String queryString = new String("from OrkLoginString as ls left join ls.user as user where ls.loginString=:ls");
			if (ContextListener.debugSwitch == false) {
				queryString = queryString + " and user.password=:password";
			}
			Query query = hbnSession.createQuery(queryString);
			query.setString("ls", username);
			if (ContextListener.debugSwitch == false) {
				query.setString("password", password);
			}
			ArrayList results = (ArrayList)query.list();
			Object[] row = (Object[])query.uniqueResult();
			if (row != null) {
				user = (OrkUser)row[1];
				logger.debug("Found userid:" + user.getId() + " for login string:" + username);
			}
		}
		catch ( HibernateException he ) {
			logger.error("login: exception:" + he.getClass().getName());
		}
		catch (Exception e)
		{
			logger.error("login: exception:", e);
		}
		finally {
			if(hbnSession != null) {hbnSession.close();}
		}
		return user;
	}

	public boolean changePassword(int userId, String oldPassword, String newPassword) {
		
		Session hbnSession = null;
		Transaction tx = null;
		OrkUser user = null;
		boolean success = false;
		
		logger.debug("Trying to change password for userid:" + userId);
		
		try
		{
			hbnSession = OrkWeb.hibernateManager.getSession();

			
			user = (OrkUser)hbnSession.get(OrkUser.class, userId);
			if(user == null) {
				logger.warn("Userid:" + userId + " does not exist");
			}
			else {
				if(user.getPassword().equals(oldPassword)) {
					tx = hbnSession.beginTransaction();
					user.setPassword(newPassword);
					hbnSession.save(user);
					tx.commit();
					success = true;
					logger.debug("Changed password for userid:" + userId);
				}
			}
		}
		catch ( HibernateException he ) {
			logger.error("changePassword: exception:" + he.getClass().getName());
		}
		catch (Exception e)
		{
			logger.error("changePassword: exception:", e);
		}
		finally {
			if(hbnSession != null) {hbnSession.close();}
		}
		return success;		
	}
}
