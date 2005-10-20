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

package net.sf.oreka.serializers;

import java.util.HashMap;

import net.sf.oreka.*;

public abstract class KeyValueSerializer extends OrkSerializer
{	
	HashMap<String, String> map;
	
	KeyValueSerializer()
	{
		map = new HashMap<String, String>();
	}
	
	String getString(String key, String oldValue, boolean required) throws OrkException
	{
		String value = map.get(key.toLowerCase());
		if (value == null)
		{
			if (required == true)
			{
				throw (new OrkException("Serializer.GetString: required parameter missing:" + key));
			}
			value = oldValue;
		}
		return value;
	}
	
	public OrkObject objectValue(String key, OrkObject value, boolean required) throws OrkException
	{
		throw (new OrkException("KeyValueSerializer.objectValue: Nested objects not allowed for key-value serializers"));
	}
}
