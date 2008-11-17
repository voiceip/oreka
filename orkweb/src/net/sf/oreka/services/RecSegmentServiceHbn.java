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
package net.sf.oreka.services;

import java.text.SimpleDateFormat;
import java.util.List;

import net.sf.oreka.Direction;
import net.sf.oreka.orkweb.OrkWeb;
import net.sf.oreka.persistent.OrkSegment;

import org.apache.log4j.Level;
import org.apache.log4j.Logger;
import org.hibernate.HibernateException;
import org.hibernate.Query;
import org.hibernate.ScrollableResults;
import org.hibernate.Session;
import org.hibernate.Transaction;

public class RecSegmentServiceHbn implements RecSegmentService{

	private boolean firstCriterium = true;
	
	static Logger logger = Logger.getLogger(RecSegmentServiceHbn.class);
	/**
	 * 
	 */
	public RecSegmentServiceHbn() {

	}
	
//	private void addCriteriumPrefix (StringBuffer query) {
//		if(firstCriterium){
//			query.append(" where ");
//			firstCriterium = false;
//		}
//		else {
//			query.append(" and ");
//		}		
//	}
	
	public int getResults(RecSegmentFilter filter, int offset, int number, String orderBy, boolean ascending, List results)
	{	
		firstCriterium = true;
		int numResults = 0;
		logger.log(Level.DEBUG, "Entering getResults");
		//logger.log(Level.INFO, System.getProperty("java.class.path"));
		
		//RecSegment seg1 = new RecSegment();
		//RecSegment seg2 = new RecSegment();
		
		//RecTape tape1 = new RecTape();
		//RecTape tape2 = new RecTape();
		/*
		RecSegmentResult result1 = new RecSegmentResult();
		RecSegmentResult result2 = new RecSegmentResult();
		
		result1.getRecSegment().setDuration(10);
		result1.getRecSegment().setLocalParty("01223");
		results.add(result1);
		result2.getRecSegment().setDuration(11);
		result2.getRecSegment().setLocalParty("01440");
		results.add(result2);
*/
		/*
		for (int i=0; i<number; i++)
		{
			RecSegmentResult result = new RecSegmentResult();
			result.getRecSegment().setDuration(offset + i);
			result.getRecSegment().setLocalParty(orderBy);
			result.getRecTape().setId(ascending ? 0:1);
			results.add(result);
		}
		numResults = 502;
		*/

		Transaction tx = null;		
		Session session = null;
		try
		{
			session = OrkWeb.hibernateManager.getSession();

			StringBuffer queryString = new StringBuffer("from OrkSegment as seg left join seg.tape as tape left join tape.service as srv ");
			//StringBuffer queryString = new StringBuffer("from RecSegment as seg ");

			//boolean firstCriterium = false;
			
			if (filter.getStartDate() != null && filter.getEndDate() != null)
				queryString.append(" where seg.timestamp between :startDate and :endDate ");
			else if (filter.getStartDate() != null)
				queryString.append(" where seg.timestamp > :startDate ");
			else if (filter.getEndDate() != null)
				queryString.append(" where seg.timestamp < :endDate ");
			
			if(filter.getLocalParty().length() > 0) {
				queryString.append(" and seg.localParty=:localParty ");
			}
			if(filter.getRemoteParty().length() > 0) {
				queryString.append(" and seg.remoteParty=:remoteParty ");
			}
			if(filter.getMinDuration().length() > 0) {
				queryString.append(" and seg.duration>:minDuration ");
			}
			if(filter.getMaxDuration().length() > 0) {
				queryString.append(" and seg.duration<:maxDuration ");
			}
			if(filter.getDirection() != Direction.ALL){
				queryString.append(" and seg.direction=:direction ");
			}
			
			if(orderBy.length() == 0) {
				orderBy = "seg.timestamp";
			}
			queryString.append(" order by ");
			queryString.append(orderBy);
			if (ascending) {
				queryString.append(" asc");			
			}
			else {
				queryString.append(" desc");	
			}
			
			Query query = session.createQuery(queryString.toString());
			
			SimpleDateFormat dateFormat = new SimpleDateFormat("yyyy-MM-dd hh:mm:ss");
			if (filter.getStartDate() != null) {
				logger.debug("Filter start date:" + dateFormat.format(filter.getStartDate()));
				query.setTimestamp("startDate", filter.getStartDate());
			}	
			if (filter.getEndDate() != null) {
				logger.debug("Filter end date:" + dateFormat.format(filter.getEndDate()));
				query.setTimestamp("endDate", filter.getEndDate());
			}	
			
			if(filter.getLocalParty().length() > 0) {
				query.setString("localParty", filter.getLocalParty());
			}
			if(filter.getRemoteParty().length() > 0) {
				query.setString("remoteParty", filter.getRemoteParty());
			}
			if(filter.getMinDuration().length() > 0) {
				query.setString("minDuration", filter.getMinDuration());
			}
			if(filter.getMaxDuration().length() > 0) {
				query.setString("maxDuration", filter.getMaxDuration());
			}
			if(filter.getDirection() != Direction.ALL){
				query.setParameter( "direction", filter.getDirection().ordinal() );
				//query.setParameter( "direction", filter.getDirection().name() );
			}
			
//			Criteria crit = session.createCriteria(RecSegment.class);
//			//crit.setFetchMode("RecTape",FetchMode.EAGER);
//			crit.setFetchMode(null, FetchMode.LAZY);
			
			ScrollableResults scrollDocs = query.scroll();
			
		
			if ( scrollDocs.last() ) {
				numResults = scrollDocs.getRowNumber()+1;
				logger.debug("Num res:" + numResults);
			}
			
			//scrollDocs.beforeFirst();
			scrollDocs.setRowNumber(offset);
			int rowsSoFar = 0;
	
			while (scrollDocs.get()!= null && rowsSoFar<number)
			{
				rowsSoFar++;
				OrkSegment seg = (OrkSegment)scrollDocs.get(0);
				
				//logger.log(Level.ERROR, seg.getRecTape().getUrl());
				
				//RecTape tape = (RecTape)scrollDocs.get(1);
				//RecTape tape = new RecTape();
				RecSegmentResult res = new RecSegmentResult();
				res.setRecSegment(seg);
				//res.setRecTape(tape);
				results.add(res);
				scrollDocs.next();
			}
		}
		catch ( HibernateException he ) {
			if ( tx != null ) tx.rollback();
			logger.log(Level.ERROR, he.toString());
			he.printStackTrace();
		}
		catch (Exception e)
		{
			logger.error(e);
			e.printStackTrace();
		}
		finally {
			session.close();
		}
		return numResults;
	}

}
