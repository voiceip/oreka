package net.sf.oreka.srvc;

import net.sf.oreka.HibernateManager;

import org.apache.log4j.Level;
import org.apache.log4j.Logger;
import org.hibernate.Criteria;
import org.hibernate.HibernateException;
import org.hibernate.ScrollableResults;
import org.hibernate.Session;
import org.hibernate.Transaction;

public class ObjectServiceHbn implements ObjectService {

	static Logger logger = Logger.getLogger(ObjectServiceHbn.class);	
	
	public Object getObjectById(java.lang.Class cl, int id) {
		
		Session hbnSession = null;
		Object obj = null;
		
		logger.debug("Retrieving " + cl.getName() +  " object with id:" + id);
		
		try
		{
			hbnSession = HibernateManager.instance().getSession();
			obj = hbnSession.get(cl, id);
		}
		catch ( HibernateException he ) {
			logger.error("getObjectById: exception:" + he.getClass().getName());
		}
		catch (Exception e)
		{
			logger.error("getObjectById: exception:", e);
		}
		finally {
			if(hbnSession != null) {hbnSession.close();}
		}
		return obj;
	}

	public void saveObject(Object obj) {
		
		Session hbnSession = null;
		Transaction tx = null;
		
		logger.debug("Saving " + obj.getClass().getName());
		
		try
		{
			hbnSession = HibernateManager.instance().getSession();
			tx = hbnSession.beginTransaction();
			hbnSession.saveOrUpdate(obj);	// was merge
			tx.commit();
		}
		catch ( HibernateException he ) {
			logger.log(Level.ERROR, he.toString());
			he.printStackTrace();
		}
		catch (Exception e)
		{
			logger.error(e);
			e.printStackTrace();
		}
		finally {
			if(hbnSession != null) {hbnSession.close();}
		}
	}
	
	public int getNumObjects(java.lang.Class cl) {
		
		Session hbnSession = null;
		int numObjets = 0;
		
		try
		{
			hbnSession = HibernateManager.instance().getSession();

			Criteria crit = hbnSession.createCriteria(cl);
			
			// figure out total number of objects returned
			ScrollableResults scrollRes = crit.scroll();
			if ( scrollRes.last() ) {
				numObjets = scrollRes.getRowNumber();
			}
		}
		catch ( HibernateException he ) {
			logger.error("getNumObjects: exception:" + he.getClass().getName());
		}
		catch (Exception e)
		{
			logger.error("getNumObjects: exception:", e);
		}
		finally {
			if(hbnSession != null) {hbnSession.close();}
		}
		
		return numObjets;
	}
}
