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

import com.codahale.metrics.annotation.Timed;
import io.astefanutti.metrics.aspectj.Metrics;
import net.sf.oreka.OrkException;
import net.sf.oreka.messages.AsyncMessage;
import net.sf.oreka.messages.SimpleResponseMessage;
import net.sf.oreka.messages.SyncMessage;
import net.sf.oreka.serializers.OrkSerializer;

@Metrics(registry = "appMetrics")
public class PingMessage extends SyncMessage {

	@Override
	@Timed(name = "process")
	public AsyncMessage process() {
		SimpleResponseMessage response = new SimpleResponseMessage();
		response.setSuccess(true);
		response.setComment("pong");
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