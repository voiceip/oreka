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

package net.sf.oreka.messages.test;

import net.sf.oreka.OrkException;
import net.sf.oreka.OrkObject;
import net.sf.oreka.serializers.OrkSerializer;

public class TestSubMessage implements OrkObject {

	String stringSubParm = "";
	int intSubParm = 0;
	
	public void define(OrkSerializer serializer) throws OrkException {
		
		stringSubParm = serializer.stringValue("stringsubparm",stringSubParm,false);
		intSubParm = serializer.intValue("intsubparm", intSubParm, false);
	}

	public String getOrkClassName() {
		
		return "testsub";
	}

	public void validate() {
		// TODO Auto-generated method stub
		
	}

	public int getIntSubParm() {
		return intSubParm;
	}

	public void setIntSubParm(int intSubParm) {
		this.intSubParm = intSubParm;
	}

	public String getStringSubParm() {
		return stringSubParm;
	}

	public void setStringSubParm(String stringSubParm) {
		this.stringSubParm = stringSubParm;
	}

}
