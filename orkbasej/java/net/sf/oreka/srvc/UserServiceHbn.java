package net.sf.oreka.srvc;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import net.sf.oreka.HibernateManager;
import net.sf.oreka.OrkBase;
import net.sf.oreka.bo.UserBo;
import net.sf.oreka.persistent.User;

import org.apache.log4j.Logger;
import org.hibernate.Criteria;
import org.hibernate.HibernateException;
import org.hibernate.Query;
import org.hibernate.ScrollableResults;
import org.hibernate.Session;
import org.hibernate.Transaction;

public class UserServiceHbn implements UserService {

	static Logger logger = Logger.getLogger(UserServiceHbn.class);	
	
	public UserBo login(String username, String password) {
		
		Session hbnSession = null;
		UserBo userBo = null;
		
		logger.debug("Trying to login user:" + username + " with passwd:" + password);
		
		try
		{
			hbnSession = HibernateManager.instance().getSession();
			
			String queryString = new String("from LoginString as ls left join ls.user as user where ls.loginString=:ls and user.deleted=0 ");
			if (OrkBase.instance().isDebugSwitch() == false) {
				queryString = queryString + " and user.password=:password";
			}
			Query query = hbnSession.createQuery(queryString);
			query.setString("ls", username);
			if (OrkBase.instance().isDebugSwitch() == false) {
				query.setString("password", password);
			}
			ArrayList results = (ArrayList)query.list();
			Object[] row = (Object[])query.uniqueResult();
			if (row != null) {
				userBo = new UserBo();
				userBo.setUser((User)row[1]);
				logger.debug("Found userid:" + userBo.getUser().getId() + " for login string:" + username);
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
		return userBo;
	}

	public boolean changePassword(int userId, String oldPassword, String newPassword) {
		
		Session hbnSession = null;
		Transaction tx = null;
		User user = null;
		boolean success = false;
		
		logger.debug("Trying to change password for userid:" + userId);
		
		try
		{
			hbnSession = HibernateManager.instance().getSession();

			
			user = (User)hbnSession.get(User.class, userId);
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

	public int getUsers(UserFilter filter, int offset, int number, String orderBy, boolean ascending, List<UserBo> results) {
		
		Session hbnSession = null;
		Transaction tx = null;
		logger.debug("Entering getUsers");
		List<User> users;
		int numResults = 0;
		
		try
		{
			hbnSession = HibernateManager.instance().getSession();

			/*
			Criteria crit = hbnSession.createCriteria(User.class);
			//crit.add( Expression.eq( "color", eg.Color.BLACK ) );
			
			// figure out total number of users returned
			ScrollableResults scrollRes = crit.scroll();
			if ( scrollRes.last() ) {
				numResults = scrollRes.getRowNumber()+1;
			}
			
			// get only one page worth of users
			crit.setMaxResults(number);
			crit.setFirstResult(offset);
			users = crit.list();
			
			Iterator it = users.iterator();
			while(it.hasNext()) {
				UserBo ubo = new UserBo();
				ubo.setUser((User)it.next());
				results.add(ubo);
			}
			*/
			
			StringBuffer queryString = new StringBuffer("from User as user where user.deleted=0 ");
			Query query = hbnSession.createQuery(queryString.toString());
			
			ScrollableResults scrollRes = query.scroll();
			if ( scrollRes.last() ) {
				numResults = scrollRes.getRowNumber()+1;
			}
			
			// get only one page worth of users		
			scrollRes.setRowNumber(offset);
			int numRetrieved = 0;
			
			while(scrollRes.next() && numRetrieved<number) {
				numRetrieved++;
				UserBo ubo = new UserBo();
				ubo.setUser((User)scrollRes.get()[0]);
				results.add(ubo);
			}
			
			logger.debug("getUsers: got " + numResults + " users");
		}
		catch ( HibernateException he ) {
			logger.error("getUsers: exception:" + he.getClass().getName());
		}
		catch (Exception e)
		{
			logger.error("getUsers: exception:", e);
		}
		finally {
			if(hbnSession != null) {hbnSession.close();}
		}	
		return numResults;
	}

	public void deleteUser(int userId) {
		
		Session hbnSession = null;
		Transaction tx = null;
		User user = null;
		
		logger.debug("Deleting userid:" + userId);
		
		try
		{
			hbnSession = HibernateManager.instance().getSession();

			
			user = (User)hbnSession.get(User.class, userId);
			if(user == null) {
				logger.warn("Userid:" + userId + " does not exist");
			}
			else {
				tx = hbnSession.beginTransaction();
				user.setDeleted(true);
				hbnSession.save(user);
				tx.commit();
			}
		}
		catch ( HibernateException he ) {
			logger.error("deleteUser: exception:" + he.getClass().getName());
		}
		catch (Exception e)
		{
			logger.error("deleteUser: exception:", e);
		}
		finally {
			if(hbnSession != null) {hbnSession.close();}
		}		
	}

	public void disableUser(int userId) {
		
		Session hbnSession = null;
		Transaction tx = null;
		User user = null;
		
		logger.debug("Disabling userid:" + userId);
		
		try
		{
			hbnSession = HibernateManager.instance().getSession();
			user = (User)hbnSession.get(User.class, userId);
			if(user == null) {
				logger.warn("Userid:" + userId + " does not exist");
			}
			else {
				tx = hbnSession.beginTransaction();
				user.setDisabled(true);
				hbnSession.save(user);
				tx.commit();
			}
		}
		catch ( HibernateException he ) {
			logger.error("disableUser: exception:" + he.getClass().getName());
		}
		catch (Exception e)
		{
			logger.error("disableUser: exception:", e);
		}
		finally {
			if(hbnSession != null) {hbnSession.close();}
		}	
	}
}
