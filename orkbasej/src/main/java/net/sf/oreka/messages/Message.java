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

package net.sf.oreka.messages;

import net.sf.oreka.*;
import net.sf.oreka.serializers.OrkSerializer;

public abstract class Message implements OrkObject {
	
	String hostname = "";
	
	public void defineMessage(OrkSerializer serializer) throws OrkException {
		
		hostname = serializer.stringValue("hostname", hostname, false);
	}

	public String getHostname() {
		return hostname;
	}
	

	public void setHostname(String hostname) {
		this.hostname = hostname;
	}
	
}
