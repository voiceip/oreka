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

import java.io.Serializable;
import java.util.Date;

import javax.persistence.Entity;
import javax.persistence.GenerationType;
import javax.persistence.GeneratedValue;
import javax.persistence.Id;
import javax.persistence.Table;

/**
 * @hibernate.class
 */
@Entity
@Table(name = "orkuser")
public class OrkUser implements Serializable {
	
	static final long serialVersionUID = 1l;
	private int id = 0;
	private String password = "";
	private String firstname = "";
	private String lastname = "";
	private String email = "";
	private boolean deleted = false;
	private boolean disabled = false;
	private Date dateCreated = new Date(0);
	private Date dateDisabled = new Date(0);
	private Date dateDeleted = new Date(0);
	
	//private Set<LoginString> loginStrings;
	  
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
	@Id @GeneratedValue(strategy=GenerationType.IDENTITY)
	public int getId() {
		return id;
	}
	

	/**
	 * @param id The id to set.
	 */
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
	public OrkUser() {
		//loginStrings = new HashSet<LoginString>();
	}

	public Date getDateCreated() {
		return dateCreated;
	}
	


	public void setDateCreated(Date dateCreated) {
		this.dateCreated = dateCreated;
	}
	


	public Date getDateDeleted() {
		return dateDeleted;
	}
	


	public void setDateDeleted(Date dateDeleted) {
		this.dateDeleted = dateDeleted;
	}
	


	public Date getDateDisabled() {
		return dateDisabled;
	}
	


	public void setDateDisabled(Date dateDisabled) {
		this.dateDisabled = dateDisabled;
	}
	


	public boolean isDeleted() {
		return deleted;
	}
	


	public void setDeleted(boolean deleted) {
		this.deleted = deleted;
	}
	


	public boolean isDisabled() {
		return disabled;
	}
	


	public void setDisabled(boolean disabled) {
		this.disabled = disabled;
	}
	
	
	
}
