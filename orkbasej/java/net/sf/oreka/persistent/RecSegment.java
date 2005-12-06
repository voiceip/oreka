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

import java.util.ArrayList;
import java.util.Collection;
import java.util.Date;

import javax.persistence.Entity;
import javax.persistence.GeneratorType;
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
public class RecSegment {
	
	private int id;
	private RecSession recSession;
	private long recSessionOffset;
	private RecTape recTape;
	private long recTapeOffset;
	private Date timestamp = new Date(0);
	private long duration;
	private String localParty = "";
	private String localEntryPoint = "";
	private String remoteParty = "";
	private Direction direction;;
	private User user;
	private String loginString = "";
	private RecPort recPort;
	private String recPortName;
	//private java.util.Set RecPrograms;
	private Collection<RecProgram> recPrograms;
	
	public RecSegment() {
		direction = Direction.UNKN;
		recPrograms = new ArrayList<RecProgram>();
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
	public long getRecTapeOffset() {
		return recTapeOffset;
	}
	
	/**
	 * @param RecTapeOffset The RecTapeOffset to set.
	 */
	public void setRecTapeOffset(long recTapeOffset) {
		this.recTapeOffset = recTapeOffset;
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
	public RecSession getRecSession() {
		return recSession;
	}
	
	/**
	 * @param RecSession The RecSession to set.
	 */
	public void setRecSession(RecSession recSession) {
		this.recSession = recSession;
	}
	
	/**
	 * @hibernate.property
	 * not-null="true"
	 * @return Returns the RecSessionOffset.
	 */
	public long getRecSessionOffset() {
		return recSessionOffset;
	}
	
	/**
	 * @param RecSessionOffset The RecSessionOffset to set.
	 */
	public void setRecSessionOffset(long recSessionOffset) {
		this.recSessionOffset = recSessionOffset;
	}

	/**
	 * @hibernate.many-to-one
	 * @return Returns the recTape.
	 */
	@ManyToOne
	public RecTape getRecTape() {
		return recTape;
	}
	

	/**
	 * @param recTape The recTape to set.
	 */
	public void setRecTape(RecTape recTape) {
		this.recTape = recTape;
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
    		table=@Table(name="ProgToSeg"),
    		joinColumns={@JoinColumn(name="SegId")},
    		inverseJoinColumns={@JoinColumn(name="ProgId")}
    )
	public Collection<RecProgram> getRecPrograms() {
		return recPrograms;
	}
	

	/**
	 * @param recPrograms The recPrograms to set.
	 */
	public void setRecPrograms(Collection<RecProgram> recPrograms) {
		this.recPrograms = recPrograms;
	}

	/**
	 * @hibernate.many-to-one
	 * @return Returns the user.
	 */
	@ManyToOne
	public User getUser() {
		return user;
	}
	

	/**
	 * @param user The user to set.
	 */
	public void setUser(User user) {
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
	public RecPort getRecPort() {
		return recPort;
	}

	public void setRecPort(RecPort port) {
		this.recPort = port;
	}

	public String getRecPortName() {
		return recPortName;
	}
	

	public void setRecPortName(String recPortName) {
		this.recPortName = recPortName;
	}
	
	
	
	
}
