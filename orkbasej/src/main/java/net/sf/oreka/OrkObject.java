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

package net.sf.oreka;

import net.sf.oreka.serializers.*;

public interface OrkObject {

	public String getOrkClassName();
	public void define(OrkSerializer serializer) throws OrkException;
	public void validate();
}
