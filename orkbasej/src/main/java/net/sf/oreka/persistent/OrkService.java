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
import javax.persistence.Transient;

import lombok.ToString;
import net.sf.oreka.ServiceClass;
import net.sf.oreka.util.TomcatServerXMLParser;

/**
 * @hibernate.class
 */
@Entity
@Table(name = "orkservice")
@ToString
public class OrkService implements Serializable {
	
	static final long serialVersionUID = 1l;
	private int id;
	private String name = "";
	private String hostname = "";
	private int tcpPort = 0;
	private String fileServeProtocol = "";
	private int fileServeTcpPort = 0;
	private String fileServePath = "";
	private ServiceClass serviceClass;
	private boolean recordMaster = false;
	private String contextPath = ""; 
	  
	/**
	 * 
	 */
	public OrkService() {
		serviceClass = ServiceClass.UNKN;
		contextPath = TomcatServerXMLParser.getAudioContextPath();
	}
	
	/**
	 * 
	 * @hibernate.property
	 * not-null="true"
	 * @return Returns the fileServePath.
	 */
	public String getFileServePath() {
		if (fileServePath==null)
			fileServePath="";
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

	public String getContextPath() {
		if (contextPath==null)
			contextPath="";
		return contextPath;
	}

	public void setContextPath(String contextPath) {
		this.contextPath = contextPath;
	}

	@Transient
	public String getContextPathWithPrefix() {
		
		String contextPathWithPrefix = getContextPath(); 

		// Add / prefix if non-existent
		if (contextPathWithPrefix.trim().length()!=0 && !contextPathWithPrefix.startsWith("/")) 
			contextPathWithPrefix = "/" + contextPathWithPrefix;

		return contextPathWithPrefix;
	}

	@Transient
	public String getFileServePathWithPrefix() {
		
		String fileServePathWithPrefix = getFileServePath();

		// Add / prefix if non-existent
		if (fileServePathWithPrefix.trim().length()!=0 && !fileServePathWithPrefix.startsWith("/"))
			fileServePathWithPrefix = "/" + fileServePathWithPrefix;
		
		return fileServePathWithPrefix;
	}

}
