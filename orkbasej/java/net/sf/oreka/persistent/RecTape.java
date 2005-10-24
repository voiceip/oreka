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

import java.util.Date;

import javax.persistence.Entity;
import javax.persistence.GeneratorType;
import javax.persistence.Id;
import javax.persistence.ManyToOne;
import javax.persistence.Transient;

import net.sf.oreka.Direction;

/**
 * @hibernate.class
 */
@Entity
public class RecTape {
	
	private int id;
	private Date timestamp = new Date(0);
	private long duration;
	private String filename = "";
	private Service service;
	private String localParty = "";
	private String localEntryPoint = "";
	private String remoteParty = "";
	private Direction direction;
	private RecPort port;
	private Date expiryTimestamp = new Date(0);
	
	public RecTape()
	{
		// Defaults
		direction = Direction.UNKN;
	}
	
	/**
	 * @hibernate.property
	 * @hibernate.column
	 * name="direction"
	 * index="direction"
	 * not-null="true"
	 * @return Returns the direction.
	 */
	public Direction getDirection() {
		return direction;
	}
	
	/**
	 * @param direction The direction to set.
	 */
	public void setDirection(Direction direction) {
		this.direction = direction;
	}
	
	/**
	 * @hibernate.property
	 * not-null="true"
	 * @return Returns the duration.
	 */
	public long getDuration() {
		return duration;
	}
	
	/**
	 * @param duration The duration to set.
	 */
	public void setDuration(long duration) {
		this.duration = duration;
	}
	
	/**
	 * @hibernate.property
	 * not-null="true"
	 * @return Returns the expiryTimestamp.
	 */
	public Date getExpiryTimestamp() {
		return expiryTimestamp;
	}
	
	/**
	 * @param expiryTimestamp The expiryTimestamp to set.
	 */
	public void setExpiryTimestamp(Date expiryTimestamp) {
		this.expiryTimestamp = expiryTimestamp;
	}
	
	/**
	 * @hibernate.property
	 * not-null="true"
	 * @return Returns the filename.
	 */
	public String getFilename() {
		return filename;
	}
	
	/**
	 * @param filename The filename to set.
	 */
	public void setFilename(String filename) {
		this.filename = filename;
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
	public void setId(int id) {
		this.id = id;
	}
	
	/**
	 * @hibernate.property
	 * not-null="true"
	 * @return Returns the localParty.
	 */
	public String getLocalParty() {
		return localParty;
	}
	
	/**
	 * @param localParty The localParty to set.
	 */
	public void setLocalParty(String localParty) {
		this.localParty = localParty;
	}
	
	/**
	 * @hibernate.property
	 * not-null="true"
	 * @return Returns the port.
	 */
	@ManyToOne
	public RecPort getPort() {
		return port;
	}
	
	/**
	 * @param port The port to set.
	 */
	public void setPort(RecPort port) {
		this.port = port;
	}
	
	/**
	 * @hibernate.property
	 * not-null="true"
	 * @return Returns the remoteParty.
	 */
	public String getRemoteParty() {
		return remoteParty;
	}
	
	/**
	 * @param remoteParty The remoteParty to set.
	 */
	public void setRemoteParty(String remoteParty) {
		this.remoteParty = remoteParty;
	}
	
	/**
	 * @hibernate.property
	 * not-null="true"
	 * @return Returns the timestamp.
	 */
	public Date getTimestamp() {
		return timestamp;
	}
	
	/**
	 * @param timestamp The timestamp to set.
	 */
	public void setTimestamp(Date timestamp) {
		this.timestamp = timestamp;
	}

	/**
	 * @hibernate.many-to-one
	 * @return Returns the service.
	 */
	@ManyToOne
	public Service getService() {
		return service;
	}
	

	/**
	 * @param server The service to set.
	 */
	public void setService(Service service) {
		this.service = service;
	}

	public String getLocalEntryPoint() {
		return localEntryPoint;
	}

	public void setLocalEntryPoint(String localEntryPoint) {
		this.localEntryPoint = localEntryPoint;
	}
	
	@Transient
	public long getStopTime() {
		return timestamp.getTime() + duration;
	}
	
	@Transient
	public String getUrl() {
		
		if(service != null) {
			return service.getFileServeProtocol() + "://" + 
			service.getHostname() + ":" + service.getFileServeTcpPort() + 
			"/" + service.getFileServePath() + "/" + filename; 
		}
		else {
			return "";
		}
	}
	
	@Transient
	public String getPlayUrl() {
		
		return "javascript:play('" + getUrl() + "')";
	}
	
}
