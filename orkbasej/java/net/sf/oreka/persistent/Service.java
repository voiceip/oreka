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
import javax.persistence.GeneratorType;
import javax.persistence.Id;

import net.sf.oreka.ServiceClass;

/**
 * @hibernate.class
 */
@Entity
public class Service implements Serializable {
	
	private int id;
	private String name = "";
	private String hostname = "";
	private int tcpPort = 0;
	private String fileServeProtocol = "";
	private int fileServeTcpPort = 0;
	private String fileServePath = "";
	private ServiceClass serviceClass;
	private boolean recordMaster = false;
	  
	/**
	 * 
	 */
	public Service() {
		serviceClass = ServiceClass.UNKN;
	}
	
	/**
	 * 
	 * @hibernate.property
	 * not-null="true"
	 * @return Returns the fileServePath.
	 */
	public String getFileServePath() {
		return fileServePath;
	}
	

	/**
	 * @param fileServePath The fileServePath to set.
	 */
	public void setFileServePath(String fileServePath) {
		this.fileServePath = fileServePath;
	}

	public int getFileServeTcpPort() {
		return fileServeTcpPort;
	}

	public void setFileServeTcpPort(int fileServeTcpPort) {
		this.fileServeTcpPort = fileServeTcpPort;
	}

	public int getTcpPort() {
		return tcpPort;
	}

	public void setTcpPort(int tcpPort) {
		this.tcpPort = tcpPort;
	}

	/**
	 * @hibernate.property
	 * not-null="true"
	 * @return Returns the fileServeProtocol.
	 */
	public String getFileServeProtocol() {
		return fileServeProtocol;
	}
	

	/**
	 * @param fileServeProtocol The fileServeProtocol to set.
	 */
	public void setFileServeProtocol(String fileServeProtocol) {
		this.fileServeProtocol = fileServeProtocol;
	}
	

	/**
	 * @hibernate.property
	 * unique="true"
	 * not-null="true"
	 * @return Returns the gUID.
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
	 * @return Returns the hostname.
	 */
	public String getHostname() {
		return hostname;
	}
	

	/**
	 * @param hostname The hostname to set.
	 */
	public void setHostname(String hostname) {
		this.hostname = hostname;
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

	public ServiceClass getServiceClass() {
		return serviceClass;
	}

	public void setServiceClass(ServiceClass serviceClass) {
		this.serviceClass = serviceClass;
	}

	public boolean isRecordMaster() {
		return recordMaster;
	}

	public void setRecordMaster(boolean cdrMaster) {
		this.recordMaster = cdrMaster;
	}
	

}
