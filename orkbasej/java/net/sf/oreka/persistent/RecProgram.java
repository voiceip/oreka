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

import java.util.Collection;
import java.util.Date;

import javax.persistence.Entity;
import javax.persistence.GeneratorType;
import javax.persistence.Id;
import javax.persistence.JoinColumn;
import javax.persistence.JoinTable;
import javax.persistence.ManyToMany;
import javax.persistence.ManyToOne;
import javax.persistence.OneToOne;
import javax.persistence.Table;

import net.sf.oreka.Cycle;
import net.sf.oreka.Direction;

/**
 * @hibernate.class
 */
@Entity
public class RecProgram {

	private int id;
	private String name = "";
	private String description = "";
	private boolean discarded = false;
	private Date timestamp;
	private User owner;
	private boolean active = true;
	private Direction direction = Direction.ALL;
	private int minDuration = 0;
	private int maxDuration = 0;
	private double randomPercent = 0.0;
	private Cycle cycle = Cycle.PERMANENT;
	private int startDay = 0;
	private int stopDay = 0;
	private Date startTime;
	private Date stopTime;
	private int recPerCycle = 0;
	private int recordedSoFar = 0;
	private String localParty = "";
	private String remoteParty = "";
	private int keepForHours = 0;
	private User targetUser;
	private RecPort targetPort;
	
	//private Set RecSegments;
	private Collection<RecSegment> recSegments;
	
	/**
	 * @hibernate.set
	 *  lazy="true"
	 *  table="PrgToSeg"
	 * @hibernate.collection-key
	 *  column="RecProgram"
	 * @hibernate.collection-many-to-many
	 *  column="RecSegment"
	 *  class="net.sf.oreka.persistent.RecSegment"
	 * @return Returns the recSegments.
	 */
    @ManyToMany
//    (
//    		targetEntity="net.sf.oreka.persistent.RecSegment"
//    )
    @JoinTable(
    		table=@Table(name="ProgToSeg"),
    		joinColumns={@JoinColumn(name="ProgId")},
    		inverseJoinColumns={@JoinColumn(name="SegId")}
    )
	public Collection<RecSegment> getRecSegments() {
		return recSegments;
	} 
	

	/**
	 * @param recSegments The recSegments to set.
	 */
	public void setRecSegments(Collection<RecSegment> recSegments) {
		this.recSegments = recSegments;
	}
	

	public RecProgram () {
		direction = Direction.ALL;
	}

	/**
	 * @hibernate.property
	 * not-null="true"
	 * @return Returns the active.
	 */
	public boolean isActive() {
		return active;
	}
	

	/**
	 * @param active The active to set.
	 */
	public void setActive(boolean active) {
		this.active = active;
	}
	

	/**
	 * @hibernate.property
	 * not-null="true"
	 * @return Returns the cycle.
	 */
	public Cycle getCycle() {
		return cycle;
	}
	

	/**
	 * @param cycle The cycle to set.
	 */
	public void setCycle(Cycle cycle) {
		this.cycle = cycle;
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
	 * @hibernate.property
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
	 * @return Returns the discarded.
	 */
	public boolean isDiscarded() {
		return discarded;
	}
	

	/**
	 * @param discarded The discarded to set.
	 */
	public void setDiscarded(boolean discarded) {
		this.discarded = discarded;
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
	 * @return Returns the keepForHours.
	 */
	public int getKeepForHours() {
		return keepForHours;
	}
	

	/**
	 * @param keepForHours The keepForHours to set.
	 */
	public void setKeepForHours(int keepForHours) {
		this.keepForHours = keepForHours;
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
	 * @return Returns the maxDuration.
	 */
	public int getMaxDuration() {
		return maxDuration;
	}
	

	/**
	 * @param maxDuration The maxDuration to set.
	 */
	public void setMaxDuration(int maxDuration) {
		this.maxDuration = maxDuration;
	}
	

	/**
	 * @hibernate.property
	 * not-null="true"
	 * @return Returns the minDuration.
	 */
	public int getMinDuration() {
		return minDuration;
	}
	

	/**
	 * @param minDuration The minDuration to set.
	 */
	public void setMinDuration(int minDuration) {
		this.minDuration = minDuration;
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
	

	/**
	 * @hibernate.property
	 * not-null="true"
	 * @return Returns the randomPercent.
	 */
	public double getRandomPercent() {
		return randomPercent;
	}
	

	/**
	 * @param randomPercent The randomPercent to set.
	 */
	public void setRandomPercent(double randomPercent) {
		this.randomPercent = randomPercent;
	}
	

	/**
	 * @hibernate.property
	 * not-null="true"
	 * @return Returns the recordedSoFar.
	 */
	public int getRecordedSoFar() {
		return recordedSoFar;
	}
	

	/**
	 * @param recordedSoFar The recordedSoFar to set.
	 */
	public void setRecordedSoFar(int recordedSoFar) {
		this.recordedSoFar = recordedSoFar;
	}
	

	/**
	 * @hibernate.property
	 * not-null="true"
	 * @return Returns the recPerCycle.
	 */
	public int getRecPerCycle() {
		return recPerCycle;
	}
	

	/**
	 * @param recPerCycle The recPerCycle to set.
	 */
	public void setRecPerCycle(int recPerCycle) {
		this.recPerCycle = recPerCycle;
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
	 * @return Returns the startDay.
	 */
	public int getStartDay() {
		return startDay;
	}
	

	/**
	 * @param startDay The startDay to set.
	 */
	public void setStartDay(int startDay) {
		this.startDay = startDay;
	}
	

	/**
	 * @hibernate.property
	 * not-null="true"
	 * @return Returns the startTime.
	 */
	public Date getStartTime() {
		return startTime;
	}
	

	/**
	 * @param startTime The startTime to set.
	 */
	public void setStartTime(Date startTime) {
		this.startTime = startTime;
	}
	

	/**
	 * @hibernate.property
	 * not-null="true"
	 * @return Returns the stopDay.
	 */
	public int getStopDay() {
		return stopDay;
	}
	

	/**
	 * @param stopDay The stopDay to set.
	 */
	public void setStopDay(int stopDay) {
		this.stopDay = stopDay;
	}
	

	/**
	 * @hibernate.property
	 * not-null="true"
	 * @return Returns the stopTime.
	 */
	public Date getStopTime() {
		return stopTime;
	}
	

	/**
	 * @param stopTime The stopTime to set.
	 */
	public void setStopTime(Date stopTime) {
		this.stopTime = stopTime;
	}
	

	/**
	 * @hibernate.property
	 * not-null="true"
	 * @return Returns the targetPort.
	 */
	@OneToOne
	public RecPort getTargetPort() {
		return targetPort;
	}
	

	/**
	 * @param targetPort The targetPort to set.
	 */
	public void setTargetPort(RecPort targetPort) {
		this.targetPort = targetPort;
	}
	
	/**
	 * @hibernate.many-to-one
	 * @return Returns the targetUser.
	 */
	@ManyToOne
	public User getTargetUser() {
		return targetUser;
	}
	


	/**
	 * @param targetUser The targetUser to set.
	 */
	public void setTargetUser(User targetUser) {
		this.targetUser = targetUser;
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
	 * @return Returns the owner.
	 */
	@ManyToOne
	public User getOwner() {
		return owner;
	}
	


	/**
	 * @param owner The owner to set.
	 */
	public void setOwner(User owner) {
		this.owner = owner;
	}
}
