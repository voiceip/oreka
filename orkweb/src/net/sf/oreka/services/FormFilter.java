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

public abstract class FormFilter {

	boolean isValid = true;

	/**
	 * @return Returns the isValid.
	 */
	public boolean isValid() {
		return isValid;
	}
	

	/**
	 * @param isValid The isValid to set.
	 */
	public void setValid(boolean isValid) {
		this.isValid = isValid;
	}
	
	
}
