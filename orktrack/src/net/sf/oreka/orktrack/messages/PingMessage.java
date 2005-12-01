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

package net.sf.oreka.orktrack.messages;

import net.sf.oreka.OrkException;
import net.sf.oreka.messages.AsyncMessage;
import net.sf.oreka.messages.SimpleResponseMessage;
import net.sf.oreka.messages.SyncMessage;
import net.sf.oreka.serializers.OrkSerializer;

import org.apache.log4j.Logger;

public class PingMessage extends SyncMessage {

	static Logger logger = Logger.getLogger(PingMessage.class);
	
	
	public PingMessage() {
	}
	
	@Override
	public AsyncMessage process() {
		
		SimpleResponseMessage response = new SimpleResponseMessage();
		response.setSuccess(true);
		return response;
	}

	public void define(OrkSerializer serializer) throws OrkException {
		
	}

	public String getOrkClassName() {
		return "ping";
	}

	public void validate() {
		// TODO Auto-generated method stub
		
	}

}