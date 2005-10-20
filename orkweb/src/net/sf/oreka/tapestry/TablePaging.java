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
package net.sf.oreka.tapestry;

import java.util.*;
import org.apache.log4j.*;
import org.apache.tapestry.*;


public abstract class TablePaging extends BaseComponent {
	
	static Logger logger = Logger.getLogger(TablePaging.class);
	
	public abstract TableState getTableState();
	public abstract void setTableState(TableState state);

	Integer sliderPosition;
	
	public void firstPageAction(IRequestCycle cycle) {
		getTableState().setNumResults(0);
		setTableState(getTableState());	// persist !
	}

	public void previousPageAction(IRequestCycle cycle) {
		TableState ps = getTableState();
		if ( (ps.getCurrentOffset() - ps.getResultsPerPage()) >= 0 ) {
			ps.setCurrentOffset(ps.getCurrentOffset() - ps.getResultsPerPage());
		}		
		setTableState(getTableState());	// persist !
	}
	
	public void nextPageAction(IRequestCycle cycle) {
		  //logger.log(Level.INFO, "before:" + getPagingState().getCurrentPage());
		  //getPagingState().setCurrentPage(getPagingState().getCurrentPage()+1);
		  //logger.log(Level.INFO, "after:" + getPagingState().getCurrentPage());
		
		TableState ps = getTableState();
		if ( (ps.getCurrentOffset() + ps.getResultsPerPage()) <  ps.getNumResults() ) {
			ps.setCurrentOffset(ps.getCurrentOffset() + ps.getResultsPerPage());
		}
		setTableState(getTableState());	// persist !
	 }
	
	public void lastPageAction(IRequestCycle cycle) {
		
		TableState ps = getTableState();
		double numPages = (double)ps.getNumResults() / ps.getResultsPerPage();
		numPages = java.lang.Math.ceil(numPages);
		//int numWholePages = ps.getNumResults() / ps.getResultsPerPage();
		//int remainder = ps.getNumResults() % ps.getResultsPerPage();
		//int lastPageOffset = 0;
		//if (remainder > 0) {
		//	lastPageOffset = numWholePages * ps.getResultsPerPage();
		//}
		//else if (numWholePages > 0){
		//	lastPageOffset = (numWholePages - 1) * ps.getResultsPerPage();
		//}
		int lastPageOffset = 0;
		if (numPages >= 1) {
			lastPageOffset = (int)(numPages-1) * ps.getResultsPerPage();
		}
		ps.setCurrentOffset(lastPageOffset);
		setTableState(getTableState());	// persist !
	}
	
	public void sliderClickAction(IRequestCycle cycle) {
		
        Object[] parameters = cycle.getServiceParameters();
		Integer sliderPosition = (Integer)parameters[0];
		
		TableState ts = getTableState();
		ts.setSliderPosition(sliderPosition);
		double newOffset = ((double)ts.getNumResults() / getSliderPositions().size()) * sliderPosition.intValue();
		ts.setCurrentOffset((int)newOffset);
		logger.log(Level.INFO, "Slider: pos:" + sliderPosition.intValue() + " offset:" + newOffset);
		setTableState(getTableState());	// persist !
	}	
	
	/**
	 * @return Returns the sliderPositions.
	 */
	public List getSliderPositions() {
		
		ArrayList sliderPositions = new ArrayList();
		int numPositions = java.lang.Math.min(getTableState().getNumPages(), 20);
		for(int i=0;i<numPositions;i++) {
			sliderPositions.add(new Integer(i));
		}
		return sliderPositions;
	}
	
}
