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
import java.util.ArrayList;
import java.util.Collection;
import java.util.Date;

import javax.persistence.Entity;
import javax.persistence.GenerationType;
import javax.persistence.GeneratedValue;
import javax.persistence.Id;
import javax.persistence.JoinColumn;
import javax.persistence.JoinTable;
import javax.persistence.ManyToMany;
import javax.persistence.ManyToOne;
import javax.persistence.Table;
import javax.persistence.Transient;

import net.sf.oreka.Direction;

/**
 * @hibernate.class
 */
@Entity
@Table(name = "orksegment")
public class OrkSegment implements Serializable {
	
	static final long serialVersionUID = 1l;
	private int id;
	private OrkSession session;
	private long sessionOffset;
	private OrkTape tape;
	private long tapeOffset;
	private Date timestamp = new Date(0);
	private long duration;
	private String localParty = "";
	private String localEntryPoint = "";
	private String remoteParty = "";
	private Direction direction;
	private OrkUser user;
	private String loginString = "";
	private OrkPort port;
	private String portName;
	//private java.util.Set RecPrograms;
	private Collection<OrkProgram> recPrograms;
	
	public OrkSegment() {
		direction = Direction.UNKN;
		recPrograms = new ArrayList<OrkProgram>();
	}
	
	/**
	 * @hibernate.property
	 *
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
	 * @hibernate.id
	 * generator-class="native"
	 * @hibernate.collection-many-to-many
	 * column="RecSegmentId"
	 * class="RecProgram"
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
	 *
	 * @hibernate.column
	 * name="localParty"
	 * index="localParty"
	 * not-null="true"
	 * 
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
	 * @return Returns the RecTapeOffset.
	 */
	public long getTapeOffset() {
		return tapeOffset;
	}
	
	/**
	 * @param RecTapeOffset The RecTapeOffset to set.
	 */
	public void setTapeOffset(long recTapeOffset) {
		this.tapeOffset = recTapeOffset;
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
	 * @return Returns the RecSession.
	 */
	@ManyToOne
	public OrkSession getSession() {
		return session;
	}
	
	/**
	 * @param OrkSession The RecSession to set.
	 */
	public void setSession(OrkSession recSession) {
		this.session = recSession;
	}
	
	/**
	 * @hibernate.property
	 * not-null="true"
	 * @return Returns the RecSessionOffset.
	 */
	public long getSessionOffset() {
		return sessionOffset;
	}
	
	/**
	 * @param RecSessionOffset The RecSessionOffset to set.
	 */
	public void setSessionOffset(long recSessionOffset) {
		this.sessionOffset = recSessionOffset;
	}

	/**
	 * @hibernate.many-to-one
	 * @return Returns the recTape.
	 */
	@ManyToOne
	public OrkTape getTape() {
		return tape;
	}
	

	/**
	 * @param recTape The recTape to set.
	 */
	public void setTape(OrkTape recTape) {
		this.tape = recTape;
	}

	/**
	 * @hibernate.set
     *  lazy="true"
     *  table="PrgToSeg"
	 * @hibernate.collection-key
	 *  column="RecSegment"
	 * @hibernate.collection-many-to-many
	 *  column="RecProgram"
	 *  class="net.sf.oreka.persistent.RecProgram"
	 * @return Returns the recPrograms.
	 */
	@ManyToMany
    @JoinTable(
    		name="orkprogtoseg",
    		joinColumns={@JoinColumn(name="segId")},
    		inverseJoinColumns={@JoinColumn(name="progId")}
    )
	public Collection<OrkProgram> getRecPrograms() {
		return recPrograms;
	}
	

	/**
	 * @param recPrograms The recPrograms to set.
	 */
	public void setRecPrograms(Collection<OrkProgram> recPrograms) {
		this.recPrograms = recPrograms;
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

	@ManyToOne
	public OrkPort getPort() {
		return port;
	}

	public void setPort(OrkPort port) {
		this.port = port;
	}

	public String getPortName() {
		return portName;
	}
	

	public void setPortName(String recPortName) {
		this.portName = recPortName;
	}




}
