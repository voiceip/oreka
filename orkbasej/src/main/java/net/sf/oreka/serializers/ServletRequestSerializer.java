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

import javax.servlet.http.*;

import net.sf.oreka.OrkException;
import net.sf.oreka.OrkObject;
import net.sf.oreka.OrkObjectFactory;

public class ServletRequestSerializer extends OrkSerializer {

	HttpServletRequest request = null;
	
	public OrkObject deSerialize(HttpServletRequest request) throws OrkException
	{
		this.request = request;
		deserialize = true;		// Set DeSerialize mode
		
		// Instanciate the right object
		String orkClass = null;
		orkClass = request.getParameter("cmd");
		if(orkClass == null) {
			orkClass = request.getParameter("class");
			if(orkClass == null) {
				orkClass = request.getParameter("type");
				if(orkClass == null) {
					throw (new OrkException("ServletRequestSerializer.deSerialize: where is the command in:" + request.getQueryString()));			
				}
			}
		}

		OrkObject obj = OrkObjectFactory.instance().newOrkObject(orkClass.toLowerCase());
		obj.define(this);
		obj.validate();
		return obj;
	}
	
	@Override
	public void addClassName(String value) {
		; // not needed, this serializer is only a de-serializer
	}

	@Override
	void addString(String key, String value) {
		; // not needed, this serializer is only a de-serializer
	}

	@Override
	String getString(String key, String oldValue, boolean required) throws OrkException {
		String value = request.getParameter(key);
		if(value == null) {
			if (required) {
				throw (new OrkException("ServletRequestSerializer.getString: parameter not found:" + key));
			}
			else {
				value = oldValue;
			}
		}
		return value;
	}

	@Override
	public OrkObject objectValue(String key, OrkObject value, boolean required) throws OrkException {
		throw (new OrkException("ServletRequestSerializer.objectValue: Nested objects not allowed"));
	}

	
}
