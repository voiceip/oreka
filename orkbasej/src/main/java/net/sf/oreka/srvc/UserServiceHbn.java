package net.sf.oreka.srvc;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;

import net.sf.oreka.HibernateManager;
import net.sf.oreka.OrkBase;
import net.sf.oreka.OrkException;
import net.sf.oreka.bo.UserBo;
import net.sf.oreka.persistent.OrkLoginString;
import net.sf.oreka.persistent.OrkUser;

import org.apache.commons.lang.StringUtils;
import org.apache.log4j.Logger;
import org.hibernate.Criteria;
import org.hibernate.HibernateException;
import org.hibernate.Query;
import org.hibernate.ScrollableResults;
import org.hibernate.Session;
import org.hibernate.Transaction;
import org.hibernate.criterion.Expression;

public class UserServiceHbn implements UserService {

	static Logger logger = Logger.getLogger(UserServiceHbn.class);	
	
	public UserBo login(String username, String password) {
		
		Session hbnSession = null;
		UserBo userBo = null;
		
		logger.debug("Trying to login user:" + username + " with passwd:" + password);
		
		try
		{
			hbnSession = HibernateManager.instance().getSession();
			
			String queryString = new String("from OrkLoginString as ls left join ls.user as user where ls.loginString=:ls and user.deleted=0 ");
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
				userBo.setUser((OrkUser)row[1]);
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
		OrkUser user = null;
		boolean success = false;
		
		logger.debug("Trying to change password for userid:" + userId);
		
		try
		{
			hbnSession = HibernateManager.instance().getSession();

			
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

	public int getUsers(UserFilter filter, int offset, int number, String orderBy, boolean ascending, List<UserBo> results) {
		
		Session hbnSession = null;
		Transaction tx = null;
		logger.debug("Entering getUsers");
		List<OrkUser> users;
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
				numResults = scrollRes.getRowNumber();
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
			
			StringBuffer queryString = new StringBuffer("from OrkUser as user where user.deleted=0 ");
			Query query = hbnSession.createQuery(queryString.toString());
			
			ScrollableResults scrollRes = query.scroll();
			if ( scrollRes.last() ) {
				numResults = scrollRes.getRowNumber();
			}
			
			// get only one page worth of users		
			scrollRes.setRowNumber(offset);
			int numRetrieved = 0;
			
			while(scrollRes.next() && numRetrieved<number) {
				numRetrieved++;
				UserBo ubo = new UserBo();
				ubo.setUser((OrkUser)scrollRes.get()[0]);
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
		OrkUser user = null;
		
		logger.debug("Deleting userid:" + userId);
		
		try
		{
			hbnSession = HibernateManager.instance().getSession();

			
			user = (OrkUser)hbnSession.get(OrkUser.class, userId);
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
		OrkUser user = null;
		
		logger.debug("Disabling userid:" + userId);
		
		try
		{
			hbnSession = HibernateManager.instance().getSession();
			user = (OrkUser)hbnSession.get(OrkUser.class, userId);
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
	
	public void setUserLoginStrings(int userId, String loginStringsCsv) {
		
		if(loginStringsCsv == null) {
			loginStringsCsv = "";
		}
		logger.debug("Entering setUserLoginStrings; userId:" + userId + " loginStringsCsv:" + loginStringsCsv);
		
		HashSet<String> oldLoginStrings = new HashSet<String>();
		HashSet<String> newLoginStrings = new HashSet<String>();
		HashMap<String, OrkLoginString> loginStringMap = new HashMap<String, OrkLoginString>();
		
		Session hbnSession = null;
		Transaction tx = null;
		
		try
		{
			hbnSession = HibernateManager.instance().getSession();
			tx = hbnSession.beginTransaction();
		
			// 1. extract all new login strings from CSV
			String[] loginStrings = StringUtils.split(loginStringsCsv, ", ");
			for(int i=0; i<loginStrings.length; i++) {
				newLoginStrings.add(loginStrings[i]);
				
				// see if the login string exists for another user
				String queryString = new String("from OrkLoginString as ls where ls.loginString=:loginString");
				Query query = hbnSession.createQuery(queryString);
				query.setString("loginString", loginStrings[i]);
				List list = query.list();
				OrkLoginString ls = null;
				
				if(list.size() > 0) {
					ls = (OrkLoginString)list.get(0);
					if(ls != null) {
						if(ls.getUser() != null) {
							if(ls.getUser().getId() != userId && !ls.getUser().isDeleted()) {
								throw new OrkException("Login String:" + loginStrings[i] + " belongs to userid:" + ls.getUser().getId());
							}
						}
					}

				}
				// Create a new login string if it does not exist yet in the DB
				if(ls == null) {
					ls = new OrkLoginString();
					ls.setLoginString(loginStrings[i]);
					hbnSession.save(ls);
				}
				loginStringMap.put(loginStrings[i], ls);
			}
			
			// 2. get the user and extract all old login strings
			OrkUser user = (OrkUser)hbnSession.get(OrkUser.class, userId);
			if(user == null) {
				throw new OrkException("UserId:" + userId + " does not exist");			
			}
			String queryString = new String("from OrkLoginString as ls where ls.user=:user");
			Query query = hbnSession.createQuery(queryString);
			query.setEntity("user", user);
			List list = query.list();
			Iterator it = list.iterator();
			while(it.hasNext()) {
				OrkLoginString ls = (OrkLoginString)it.next();
				oldLoginStrings.add(ls.getLoginString());
				loginStringMap.put(ls.getLoginString(), ls);
			}
			
			// Add all new login strings to the user
			it = newLoginStrings.iterator();
			while(it.hasNext()) {
				String ls = (String)it.next();
				if(oldLoginStrings.contains(ls) == false) {
					OrkLoginString lso = loginStringMap.get(ls);
					lso.setUser(user);
					logger.debug("Added loginstring:" + lso.getLoginString() + " to user:" + user.getFirstname());
				}
			}
			
			// Remove all old login strings from the user
			it = oldLoginStrings.iterator();
			while(it.hasNext()) {
				String ls = (String)it.next();
				if(newLoginStrings.contains(ls) == false) {
					OrkLoginString lso = loginStringMap.get(ls);
					lso.setUser(null);
					logger.debug("Removed loginstring:" + lso.getLoginString() + " from user:" + user.getFirstname());
				}
			}			
			
			tx.commit();
		}
		catch ( HibernateException he ) {
			logger.error("setUserLoginStrings: exception:" + he.getClass().getName());
		}
		catch ( OrkException oe ) {
			logger.error("OrkException:" + oe.getMessage());
		}
		catch (Exception e)
		{
			logger.error("setUserLoginStrings: exception:", e);
		}
		finally {
			if(hbnSession != null) {hbnSession.close();}
		}
	}

	
	public String getUserLoginStrings(int userId) {
		
		Session hbnSession = null;
		OrkUser user = null;
		String loginStringsCsv = "";
		
		try
		{
			hbnSession = HibernateManager.instance().getSession();
			user = (OrkUser)hbnSession.get(OrkUser.class, userId);
			String queryString = new String("from OrkLoginString as ls where ls.user=:user");
			Query query = hbnSession.createQuery(queryString);
			query.setEntity("user", user);
			List list = query.list();
			
			Iterator it = list.iterator();
			if(it.hasNext()) {
				loginStringsCsv = ((OrkLoginString)it.next()).getLoginString();
			}
			while(it.hasNext()) {
				loginStringsCsv += ", " + ((OrkLoginString)it.next()).getLoginString();
			}
		}
		catch ( HibernateException he ) {
			logger.error("getUserLoginStrings: exception:" + he.getClass().getName());
		}
		catch (Exception e)
		{
			logger.error("getUserLoginStrings: exception:", e);
		}
		finally {
			if(hbnSession != null) {hbnSession.close();}
		}	
		return loginStringsCsv;
	}

	public int getNumNonDisabledUsers() {
		
		Session hbnSession = null;
		int numUsers = 0;
		
		try
		{
			hbnSession = HibernateManager.instance().getSession();

			Criteria crit = hbnSession.createCriteria(OrkUser.class);
			crit.add( Expression.eq( "disabled", false ) );
			crit.add( Expression.eq( "deleted", false ) );			
			
			// figure out total number of objects returned
			ScrollableResults scrollRes = crit.scroll();
			if ( scrollRes.last() ) {
				numUsers = scrollRes.getRowNumber();
			}
		}
		catch ( HibernateException he ) {
			logger.error("getNumNonDisabledUsers: exception:" + he.getClass().getName());
		}
		catch (Exception e)
		{
			logger.error("getNumNonDisabledUsers: exception:", e);
		}
		finally {
			if(hbnSession != null) {hbnSession.close();}
		}
		
		return numUsers;
	}
	
	public OrkUser getUserByLoginString(String loginString) {
		Session hbnSession = null;
		OrkUser user = null;
		
		try
		{
			hbnSession = HibernateManager.instance().getSession();
			String queryString = new String("from OrkLoginString as ls where ls.loginstring=:ls");
			Query query = hbnSession.createQuery(queryString);
			query.setString("ls", loginString);
			List list = query.list();
			Iterator it = list.iterator();
			if (it.hasNext()) {
				OrkLoginString ls = (OrkLoginString)it.next();
				user = ls.getUser();
			}
		}
		catch ( HibernateException he ) {
			logger.error("getUserByLoginString: exception:" + he.getClass().getName());
		}
		catch (Exception e) {
			logger.error("getUserByLoginString: exception:", e);
		}
		finally {
			if(hbnSession != null) {hbnSession.close();}
		}
		return user;
	}

}
