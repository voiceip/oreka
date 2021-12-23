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

import javax.persistence.Entity;
import javax.persistence.Id;
import javax.persistence.ManyToOne;
import javax.persistence.Table;

@Entity
@Table(name = "orkportface")
public class OrkPortFace implements Serializable {

	static final long serialVersionUID = 1l;
	private OrkPort port;
	private OrkService service;
	private String name;
	
	@Id
	public String getName() {
		return name;
	}
	public void setName(String name) {
		this.name = name;
	}
	@ManyToOne
	public OrkPort getPort() {
		return port;
	}
	public void setPort(OrkPort recPort) {
		this.port = recPort;
	}
	@ManyToOne
	public OrkService getService() {
		return service;
	}
	public void setService(OrkService service) {
		this.service = service;
	}
	
}
