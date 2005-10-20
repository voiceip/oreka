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

package net.sf.oreka.orktrack;

import net.sf.oreka.OrkException;
import net.sf.oreka.OrkObject;
import net.sf.oreka.serializers.OrkSerializer;

public class OrkTrackConfig implements OrkObject {

	boolean ctiDriven = false;
	
	public void define(OrkSerializer serializer) throws OrkException {
		
		ctiDriven = serializer.booleanValue("ctidriven", ctiDriven, true);
	}

	public String getOrkClassName() {
		return "orktrackconfig";
	}

	public void validate() {
		
	}

	public boolean isCtiDriven() {
		return ctiDriven;
	}

	public void setCtiDriven(boolean ctiDriven) {
		this.ctiDriven = ctiDriven;
	}

	
}
