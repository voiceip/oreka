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

import javax.persistence.*;

/**
 * @hibernate.class
 */
@Entity
@Table(name = "orkloginstring")
public class OrkLoginString implements Serializable {

	static final long serialVersionUID = 1l;
	private int id;
	private String loginString;
	private OrkUser user;
	private OrkDomain domain;
	
	/**
	 * @hibernate.many-to-one
	 * @return Returns the domain.
	 */
	@ManyToOne
	public OrkDomain getDomain() {
		return domain;
	}
	

	/**
	 * @param domain The domain to set.
	 */
	public void setDomain(OrkDomain domain) {
		this.domain = domain;
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
	 * @return Returns the loginString.
	 */
	public String getLoginString() {
		return loginString;
	}
	

	/**
	 * @param loginString The loginString to set.
	 */
	public void setLoginString(String loginString) {
		this.loginString = loginString;
	}
	

	/**
	 * @hibernate.many-to-one
	 * @return Returns the user.
	 */
	@ManyToOne
	public OrkUser getUser() {
		return user;
	}
	

	/**
	 * @param user The user to set.
	 */
	public void setUser(OrkUser user) {
		this.user = user;
	}
	
//	public void bidirSetUser(User user) {
//		this.user = user;
//		if(user != null) {
//			user.getLoginStrings().add(this);
//		}
//	}
//
//	public void bidirRemoveUser(User user) {
//		this.user = null;
//		if(user != null) {
//			user.getLoginStrings().remove(this);
//		}
//	}
	
	/**
	 * 
	 */
	public OrkLoginString() {
	}

}
