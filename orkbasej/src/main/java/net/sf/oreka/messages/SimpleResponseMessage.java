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

import net.sf.oreka.OrkException;
import net.sf.oreka.serializers.OrkSerializer;

public class SimpleResponseMessage extends AsyncMessage {

	boolean success = false;
	String comment = "";
	
	public void define(OrkSerializer serializer) throws OrkException {

		success = serializer.booleanValue("success", success, true);
		comment = serializer.stringValue("comment", comment, false);
	}

	public String getOrkClassName() {

		return "simpleresponse";
	}

	public void validate() {
		// TODO Auto-generated method stub
		
	}

	public String getComment() {
		return comment;
	}

	public void setComment(String comment) {
		this.comment = comment;
	}

	public boolean isSuccess() {
		return success;
	}

	public void setSuccess(boolean success) {
		this.success = success;
	}

}
