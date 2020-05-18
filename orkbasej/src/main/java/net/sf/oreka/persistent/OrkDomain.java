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

import javax.persistence.Entity;
import javax.persistence.GenerationType;
import javax.persistence.GeneratedValue;
import javax.persistence.Id;
import javax.persistence.Table;

/**
 * @hibernate.class
 */
@Entity
@Table(name = "orkdomain")
public class OrkDomain implements Serializable {

	static final long serialVersionUID = 1l;
	private int id;
	private String name = "";
	private String description = "";
	
	/**
	 * 
	 */
	public OrkDomain() {
	}

	/**
	 * @hibernate.property
	 * not-null="true"
	 * @return Returns the description.
	 */
	public String getDescription() {
		return description;
	}
	

	/**
	 * @param description The description to set.
	 */
	public void setDescription(String description) {
		this.description = description;
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
	 * @return Returns the name.
	 */
	public String getName() {
		return name;
	}
	

	/**
	 * @param name The name to set.
	 */
	public void setName(String name) {
		this.name = name;
	}
}
