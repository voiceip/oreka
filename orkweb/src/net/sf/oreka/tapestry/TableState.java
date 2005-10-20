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

import org.apache.log4j.Logger;

public class TableState {

	static Logger logger = Logger.getLogger(TableState.class);	
	
	public static int DEFAULT_RESULTS_PER_PAGE = 10;
	
	private int NumResults;
	private int ResultsPerPage;
	private int CurrentOffset;
	private int sliderPosition;
	private String OrderBy = new String();
	private boolean Ascending = false;
	
	/**
	 * @return Returns the ascending.
	 */
	public boolean isAscending() {
		return Ascending;
	}
	

	/**
	 * @param ascending The ascending to set.
	 */
	public void setAscending(boolean ascending) {
		Ascending = ascending;
	}

	/**
	 * @return Returns the orderBy.
	 */
	public String getOrderBy() {
		return OrderBy;
	}
	

	/**
	 * @param orderBy The orderBy to set.
	 */
	public void setOrderBy(String orderBy) {
		OrderBy = orderBy;
	}
	

	/**
	 * 
	 */ 
	public TableState() {
		ResultsPerPage = DEFAULT_RESULTS_PER_PAGE;
	}
	
	/**
	 * @return Returns the currentOffset.
	 */
	public int getCurrentOffset() {
		return CurrentOffset;
	}
	

	/**
	 * @param currentOffset The currentOffset to set.
	 */
	public void setCurrentOffset(int currentOffset) {
		CurrentOffset = currentOffset;
	}
	

	/**
	 * @return Returns the numResults.
	 */
	public int getNumResults() {
		return NumResults;
	}
	

	/**
	 * @param numResults The numResults to set.
	 */
	public void setNumResults(int numResults) {
		if (NumResults != numResults){
			NumResults = numResults;
			CurrentOffset = 0;		// reset the offset if different resultset
		}
	}
	

	/**
	 * @return Returns the resultsPerPage.
	 */
	public int getResultsPerPage() {
		return ResultsPerPage;
	}
	

	/**
	 * @param resultsPerPage The resultsPerPage to set.
	 */
	public void setResultsPerPage(int resultsPerPage) {
		ResultsPerPage = resultsPerPage;
	}
	
	public int getNumPages() {
		double numPages = (double)getNumResults() / getResultsPerPage();
		return (int)java.lang.Math.ceil(numPages);
	}
	
	public int getCurrentPage() {
		return (getCurrentOffset() / getResultsPerPage())+1;
	}


	public int getSliderPosition() {
		return sliderPosition;
	}
	


	public void setSliderPosition(int sliderPosition) {
		this.sliderPosition = sliderPosition;
	}
	

}
