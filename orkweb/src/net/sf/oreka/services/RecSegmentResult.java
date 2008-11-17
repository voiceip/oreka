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
package net.sf.oreka.services;

import net.sf.oreka.persistent.*;


public class RecSegmentResult {

	private OrkSegment RecSegment;
	private OrkTape RecTape;
	
	private String foo = "henri";

	public RecSegmentResult() {
		RecSegment = new OrkSegment();
		RecTape = new OrkTape();
	}

	/**
	 * @return Returns the recSegment.
	 */
	public OrkSegment getRecSegment() {
		return RecSegment;
	}
	

	/**
	 * @param recSegment The recSegment to set.
	 */
	public void setRecSegment(OrkSegment recSegment) {
		RecSegment = recSegment;
	}

	/**
	 * @return Returns the recTape.
	 */
	public OrkTape getRecTape() {
		return RecTape;
	}
	

	/**
	 * @param recTape The recTape to set.
	 */
	public void setRecTape(OrkTape recTape) {
		RecTape = recTape;
	}

	/**
	 * @return Returns the foo.
	 */
	public String getFoo() {
		return foo;
	}
	

	/**
	 * @param foo The foo to set.
	 */
	public void setFoo(String foo) {
		this.foo = foo;
	}
	
	

	

}
