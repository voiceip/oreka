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

import java.io.Serializable;
import java.util.Date;

import javax.persistence.Entity;
import javax.persistence.GeneratorType;
import javax.persistence.Id;
import javax.persistence.Transient;

/**
 * @hibernate.class
 */
@Entity
public class RecSession implements Serializable {
	private int id;
	private Date timestamp = new Date(0);
	private long duration;
	
	/**
	 * @hibernate.property
	 * not-null="true"
	 * @return
	 */
	public long getDuration() {
		return duration;
	}
	
	public void setDuration(long duration) {
		this.duration = duration;
	}
	
	/** 
	* @hibernate.id
	* generator-class="native"
	*/
	@Id(generate=GeneratorType.AUTO)
	public int getId() {
		return id;
	}
	
	
	public void setId(int id) {
		this.id = id;
	}
	
	/**
	 * @hibernate.property
	 * @return
	 */
	public Date getTimestamp() {
		return timestamp;
	}
	
	public void setTimestamp(Date timestamp) {
		this.timestamp = timestamp;
	}
	
	@Transient
	public long getStopTime() {
		return timestamp.getTime() + duration;
	}
	
	public void setStopTime(long stopTime) {
		duration = stopTime - timestamp.getTime();
	}
}
