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

package net.sf.oreka.persistent;

/**
 * @hibernate.class
 */
public class HbnXmlTestClass {

	long Id;
	String firstname = "";
	String lastname = "";

	/**
	 * @hibernate.property
	 * not-null="true"
	 * @return Returns the filename.
	 */
	public String getFirstname() {
		return firstname;
	}

	public void setFirstname(String firstname) {
		this.firstname = firstname;
	}

	/**
	 * @hibernate.property
	 * not-null="true"
	 * @return Returns the filename.
	 */
	public String getLastname() {
		return lastname;
	}

	public void setLastname(String lastname) {
		this.lastname = lastname;
	}
	
	/** 
	* @hibernate.id
	* generator-class="native"
	*/
	public long getId() {
		return Id;
	}

	public void setId(long id) {
		Id = id;
	}
}
