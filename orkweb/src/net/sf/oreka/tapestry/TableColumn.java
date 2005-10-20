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

public class TableColumn {
	
	String message;
	String key;
	boolean sortable;
	/**
	 * @return Returns the isSortable.
	 */
	public boolean isSortable() {
		return sortable;
	}
	

	/**
	 * @param isSortable The isSortable to set.
	 */
	public void setSortable(boolean sortable) {
		this.sortable = sortable;
	}
	

	/**
	 * @return Returns the key.
	 */
	public String getKey() {
		return key;
	}
	
	/**
	 * @param key The key to set.
	 */
	public void setKey(String key) {
		this.key = key;
	}
	
	/**
	 * @return Returns the message.
	 */
	public String getMessage() {
		return message;
	}
	
	/**
	 * @param message The message to set.
	 */
	public void setMessage(String message) {
		this.message = message;
	}
	
}
