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
package net.sf.oreka.pages;

import java.util.ArrayList;
import java.util.List;

import net.sf.oreka.services.RecSegmentFilter;
import net.sf.oreka.services.RecSegmentResult;
import net.sf.oreka.services.RecSegmentServiceHbn;
import net.sf.oreka.tapestry.TableState;

import org.apache.log4j.Level;
import org.apache.log4j.Logger;
import org.apache.tapestry.IRequestCycle;
import org.apache.tapestry.event.PageEvent;
import org.apache.tapestry.event.PageRenderListener;
import org.apache.tapestry.html.BasePage;

public abstract class RecSegmentsPage extends ProtectedPage implements PageRenderListener {

	static Logger logger = Logger.getLogger(RecSegmentsPage.class);

	private String Foo = "page";
	
	/**
	 * @return Returns the foo.
	 */
	public String getFoo() {
		return Foo;
	}
	
	/**
	 * @param foo The foo to set.
	 */
	public void setFoo(String foo) {
		Foo = foo;
	}
	
	public List getFields(){
		ArrayList list = new ArrayList();
		//list.add("recSegment.duration");
		//list.add("recSegment.localParty");
		list.add("ognl:foo");
		return list;
	}
	private String field;
	
	private int CurrentPage;
	private int ResultsOffset;
	private final int RESULTS_PER_PAGE = 4;
	private int NumResults;
	
	
	public abstract TableState getTableState();
	public abstract void setTableState(TableState state);
	
	public abstract RecSegmentFilter getRecSegmentFilter();
	public abstract void setRecSegmentFilter(RecSegmentFilter filter);
	
	public abstract RecSegmentResult getItem();
	public abstract void setItem(RecSegmentResult res);
	
	public abstract void setItems(List list);
	public abstract List getItems();

	public void formSubmit(IRequestCycle cycle) {
		
		//logger.debug("formSubmit");
		setRecSegmentFilter(getRecSegmentFilter());
	}
	
	public void pageBeginRender(PageEvent event) {	
		
		updateResults();
	}
	
	public void pageFirstAction(IRequestCycle cycle) {
		ResultsOffset = 0;
		setRecSegmentFilter(getRecSegmentFilter());
	}
	
	public void pagePreviousAction(IRequestCycle cycle) {
		ResultsOffset -= RESULTS_PER_PAGE;
		if (ResultsOffset < 0) {
			ResultsOffset = 0;
		}
		setRecSegmentFilter(getRecSegmentFilter());
	}	
	
	public void pageNextAction(IRequestCycle cycle) {
		ResultsOffset += RESULTS_PER_PAGE;
		if (ResultsOffset >= NumResults) {
			ResultsOffset = NumResults - RESULTS_PER_PAGE;
			if (ResultsOffset < 0) {
				ResultsOffset = 0;
			}
		}
		logger.log(Level.INFO, "Next, new offset:" + ResultsOffset);
		setRecSegmentFilter(getRecSegmentFilter());
	}	
	
	public void pageLastAction(IRequestCycle cycle) {
		ResultsOffset = NumResults - RESULTS_PER_PAGE;
		if (ResultsOffset < 0) {
			ResultsOffset = 0;
		}
		setRecSegmentFilter(getRecSegmentFilter());
	}
	/**
	 * @return Returns the resultsOffset.
	 */
	public int getResultsOffset() {
		return ResultsOffset;
	}
	
	/**
	 * @param resultsOffset The resultsOffset to set.
	 */
	public void setResultsOffset(int resultsOffset) {
		ResultsOffset = resultsOffset;
	}
	
	private void updateResults()
	{
		logger.debug("UpdateResults: orderby:" + getTableState().getOrderBy());		
		
		RecSegmentServiceHbn srv = new RecSegmentServiceHbn();
		if (getRecSegmentFilter().isValid()) {
			ArrayList results = new ArrayList();
			NumResults = srv.getResults(getRecSegmentFilter(), getTableState().getCurrentOffset(), getTableState().getResultsPerPage(), getTableState().getOrderBy(), getTableState().isAscending(), results);
			getTableState().setNumResults(NumResults);
			logger.debug("UpdateResults: Offset:" + ResultsOffset + " num results:" + NumResults);
			setItems(results);
		}
		else {
			logger.debug("UpdateResults: Invalid filter");
		}
	}
	/**
	 * @return Returns the field.
	 */
	public String getField() {
		return field;
	}
	
	/**
	 * @param field The field to set.
	 */
	public void setField(String field) {
		this.field = field;
	}
	
	
	
}
	