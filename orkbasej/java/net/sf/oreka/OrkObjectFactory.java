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
/** 
 * The ObjectFactory can be used to instanciate Objects based on class name.
 * All existing Objects must be registered to the ObjectFactory at startup.
 */
public class OrkObjectFactory {

	static OrkObjectFactory instance = null;
	java.util.HashMap<String, OrkObject> classes;
	
	private OrkObjectFactory()
	{
		classes = new java.util.HashMap<String, OrkObject>();
	}
	
	public static OrkObjectFactory instance()
	{
		if(instance == null)
		{
			instance = new OrkObjectFactory();
		}
		return instance;
	}
	
	public OrkObject newOrkObject(String className) throws OrkException
	{
		className = className.toLowerCase();
		OrkObject classObject = classes.get(className);
		OrkObject newInstance = null;
		if(classObject != null)
		{
			try
			{
				newInstance = classObject.getClass().newInstance();
			}
			catch (Exception e) 
			{
				throw new OrkException("OrkObjectFactory: Could not instanciate:" + className);
			}
		}
		else
		{
			throw new OrkException("OrkObjectFactory: class does not exist:" + className);
		}
		return newInstance;
	}
	
	public void registerOrkObject(OrkObject object)
	{
		classes.put(object.getOrkClassName(), object);
	}

}
