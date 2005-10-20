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

import java.util.Date;

import net.sf.oreka.Direction;
import net.sf.oreka.orkweb.ContextListener;

import org.apache.log4j.Logger;

public class RecSegmentFilter extends FormFilter {

	static Logger logger = Logger.getLogger(RecSegmentFilter.class);
	
	private String minDuration = "";
	private String minDurationError;
	private String maxDuration = "";
	private String maxDurationError;
	private String localParty = "";
	private String localPartyError;
	private String remoteParty = "";
	private String remotePartyError;
	private Date startDate = null;
    private Date endDate = null;
	private Direction direction;

	private final String UNALLOWED_CHARACTER = "Unallowed character";
	private final String ENTER_A_NUMBER = "Enter a number";
	
	private boolean isStringInvalidParty(String party) {
		// See if there are any characters not listed here
		return (party.matches(".*[^A-Za-z0-9#\\*@].*"));
	}
	
	public String getErrorStyle(){
		return "error";
	}
	public String getOkStyle(){
		return "";
	}
	
	/**
	 * @return Returns the durationError.
	 */
	public String getMinDurationError() {
		return minDurationError;
	}
	


	/**
	 * @param durationError The durationError to set.
	 */
	public void setMinDurationError(String durationError) {
		this.minDurationError = durationError;
	}
	


	/**
	 * @return Returns the localPartyError.
	 */
	public String getLocalPartyError() {
		return localPartyError;
	}
	


	/**
	 * @param localPartyError The localPartyError to set.
	 */
	public void setLocalPartyError(String localPartyError) {
		this.localPartyError = localPartyError;
	}
	


	/**
	 * @return Returns the localParty.
	 */
	public String getLocalParty() {
		return localParty;
	}
	

	/**
	 * @param localParty The localParty to set.
	 */
	public void setLocalParty(String localParty) {
		
		if(localParty == null) {
			this.localParty = "";
		}
		else {
			localParty.trim();
			this.localParty = localParty;
		}
		if (isStringInvalidParty(this.localParty)) {
			setLocalPartyError(UNALLOWED_CHARACTER);
		}
		else {
			setLocalPartyError(null);
		}
	}
	

	public RecSegmentFilter() {
		
		minDuration = new String();
		localParty = new String();
		direction = Direction.ALL;
		
		long millisIn24hours = 24 * 3600 * 1000;
		endDate = new Date();
		
		// For now, retrieve 24 hours worth of records by default
		if (ContextListener.debugSwitch == false) {
			startDate = new Date(endDate.getTime() - millisIn24hours);
		}
		else {
			startDate = new Date(0);
		}
		
		/*
	    // Read properties file.
	    Properties properties = new Properties();
	    try {
	        properties.load(new FileInputStream("RecSegmentFilter.properties"));
	    } catch (IOException e) {
			System.out.println("@@@@@@@@@@");
	    }
		*/

	}


	/**
	 * @return Returns the duration.
	 */
	public String getMinDuration() {
		return minDuration;
	}
	


	/**
	 * @param minDuration The duration to set.
	 */
	public void setMinDuration(String minDuration) {
		
		if(minDuration == null) {
			this.minDuration = "";
		}
		else {
			minDuration.trim();
			this.minDuration = minDuration;
		}
		if (this.minDuration.matches(".*[^0-9].*")) {
			// there is a funny character here ...
			setMinDurationError(ENTER_A_NUMBER);
		}
		else {
			setMinDurationError(null);
		}
	}
	public Date getEndDate() {
		return endDate;
	}
	
	public void setEndDate(Date endDate) {
		this.endDate = endDate;
	}
	
	public Date getStartDate() {
		return startDate;
	}
	
	public void setStartDate(Date startDate) {
		this.startDate = startDate;
	}

	public String getRemoteParty() {
		return remoteParty;
	}
	

	public void setRemoteParty(String remoteParty) {
		if(remoteParty == null) {
			this.remoteParty = "";
		}
		else {
			remoteParty.trim();
			this.remoteParty = remoteParty;
		}
		if (isStringInvalidParty(this.remoteParty)) {
			setRemotePartyError(UNALLOWED_CHARACTER);
		}
		else {
			setRemotePartyError(null);
		}
	}
	

	public String getRemotePartyError() {
		return remotePartyError;
	}
	

	public void setRemotePartyError(String remotePartyError) {
		this.remotePartyError = remotePartyError;
	}

	public Direction getDirection() {
		return direction;
	}
	

	public void setDirection(Direction direction) {
		this.direction = direction;
	}

	public String getMaxDuration() {
		return maxDuration;
	}
	

	public void setMaxDuration(String maxDuration) {
		if(maxDuration == null) {
			this.maxDuration = "";
		}
		else {
			maxDuration.trim();
			this.maxDuration = maxDuration;
		}
		if (this.maxDuration.matches(".*[^0-9].*")) {
			// there is a funny character here ...
			setMaxDurationError(ENTER_A_NUMBER);
		}
		else {
			setMaxDurationError(null);
		}
	}
	

	public String getMaxDurationError() {
		return maxDurationError;
	}
	

	public void setMaxDurationError(String maxDurationError) {
		this.maxDurationError = maxDurationError;
	}
	
	
	
	
	

}
