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
package net.sf.oreka.persistent;

import javax.persistence.Entity;
import javax.persistence.GeneratorType;
import javax.persistence.Id;

/**
 * @hibernate.class
 */
@Entity
public class User {
	
	private int id;
	private String password = "";
	private String firstname = "";
	private String lastname = "";
	private String email = "";
	  
	/**
	 * @hibernate.property
	 * not-null="true"
	 * @return Returns the email.
	 */
	public String getEmail() {
		return email;
	}
	

	/**
	 * @param email The email to set.
	 */
	public void setEmail(String email) {
		this.email = email;
	}
	

	/**
	 * @hibernate.property
	 * not-null="true"
	 * @return Returns the firstname.
	 */
	public String getFirstname() {
		return firstname;
	}
	

	/**
	 * @param firstname The firstname to set.
	 */
	public void setFirstname(String firstname) {
		this.firstname = firstname;
	}
	

	/**
	 * @hibernate.id
	 * generator-class="native"
	 * @return Returns the id.
	 */
	@Id(generate=GeneratorType.AUTO)
	public int getId() {
		return id;
	}
	

	/**
	 * @param id The id to set.
	 */
	@Id(generate=GeneratorType.AUTO)
	public void setId(int id) {
		this.id = id;
	}
	

	/**
	 * @hibernate.property
	 * not-null="true"
	 * @return Returns the lastname.
	 */
	public String getLastname() {
		return lastname;
	}
	

	/**
	 * @param lastname The lastname to set.
	 */
	public void setLastname(String lastname) {
		this.lastname = lastname;
	}
	

	/**
	 * @hibernate.property
	 * not-null="true"
	 * @return Returns the password.
	 */
	public String getPassword() {
		return password;
	}
	

	/**
	 * @param password The password to set.
	 */
	public void setPassword(String password) {
		this.password = password;
	}
	

	/**
	 * 
	 */
	public User() {
	}
	
}
