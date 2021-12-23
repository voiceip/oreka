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

import net.sf.oreka.*;

/**
 * 
 * Base class for all serializers.
 * Serializers have to implement their own serialize and deserialize methods
 * since the data can be read from and written to many different formats
 * 
 * Enum and Boolean value are case insensitive (all enum values must be defined 
 * lower case in the project, otherwise they won't be recognized at all)
 *
 */
public abstract class OrkSerializer {
	
	boolean deserialize = false;
	String classKey = "class";
	
	public void setClassKey(String classKey)
	{
		this.classKey = classKey;
	}
	
	public int intValue(String key, int value, boolean required) throws OrkException
	{
		if (deserialize)
		{
			return getInt(key, value, required);
		}
		else
		{
			addInt(key, value);
			return value;
		}
	}

	public double doubleValue(String key, double value, boolean required) throws OrkException
	{
		if (deserialize)
		{
			return getDouble(key, value, required);
		}
		else
		{
			addDouble(key, value);
			return value;
		}		
	}
	
	public String stringValue(String key, String value, boolean required ) throws OrkException
	{
		if (deserialize)
		{
			return getString(key, value, required);
		}
		else
		{
			addString(key, value);
			return value;
		}
	}
	
	public boolean booleanValue(String key, boolean value, boolean required ) throws OrkException
	{
		if (deserialize)
		{
			return getBoolean(key, value, required);
		}
		else
		{
			addBoolean(key, value);
			return value;
		}
	}
	
	public Enum enumValue(String key, Enum value, boolean required ) throws OrkException
	{
		if (deserialize)
		{
			return getEnum(key, value, required);
		}
		else
		{
			addEnum(key, value);
			return value;
		}
	}
	
	public abstract OrkObject objectValue(String key, OrkObject value, boolean required ) throws OrkException;
	
	public abstract void addClassName(String value);
	
	void addInt(String key, int value)
	{
		String valueString = String.valueOf(value);
		addString(key, valueString);
	}
	
	void addDouble(String key, double value)
	{
		String valueString = String.valueOf(value);
		addString(key, valueString);
	}
	
	void addBoolean(String key, boolean value)
	{
		if(value)
		{
			addString(key, "true");
		}
		else
		{
			addString(key, "false");
		}
	}
	
	void addEnum(String key, Enum value)
	{
		addString(key, value.name());
	}
	
	abstract void addString(String key, String value);

	int getInt(String key, int oldValue, boolean required) throws OrkException
	{
		int value = 0;
		String stringValue = getString(key, null, required);
		
		if (stringValue == null)
		{
			value = oldValue;
		}
		else
		{
			try
			{
				value = Integer.valueOf(stringValue);
			}
			catch (Exception e)
			{
				throw (new OrkException("Serializer: value:" + stringValue + " not an int for key:" + key));
			}
		}
		return value;
	}
	
	double getDouble(String key, double oldValue, boolean required) throws OrkException
	{
		double value = 0.0;
		String stringValue = getString(key, null, required);
		
		if (stringValue == null)
		{
			value = oldValue;
		}
		else
		{
			try
			{
				value = Double.valueOf(stringValue);
			}
			catch (Exception e)
			{
				throw (new OrkException("Serializer: value:" + stringValue + " not a double for key:" + key));
			}
		}
		return value;
	}
	
	boolean getBoolean(String key, boolean oldValue, boolean required) throws OrkException
	{
		boolean value;
		String stringValue = getString(key, null, required);

		if (stringValue == null)
		{
			value = oldValue;
		}
		else
		{
			stringValue = stringValue.toLowerCase();
			
			if(	stringValue.equals("true") || stringValue.equals("yes") || stringValue.equals("1") )
			{
				value = true;
			}
			else if( stringValue.equals("false") || stringValue.equals("no") || stringValue.equals("0") )
			{
				value = false;
			}
			else
			{
				throw(new OrkException("Serializer: Invalid boolean value:" + stringValue + " for key:" + key));
			}
		}
		return value;
	}

	Enum getEnum(String key, Enum oldValue, boolean required) throws OrkException
	{
		Enum value = null;
		String stringValue = getString(key, null, required);

		if (stringValue == null)
		{
			value = oldValue;
		}
		else
		{
			stringValue = stringValue.toUpperCase();
			Class valueClass = oldValue.getClass();
			try
			{
				value = Enum.valueOf(valueClass, stringValue);
			}
			catch (IllegalArgumentException e)
			{
				throw (new OrkException("Serializer: Invalid enum value:" + stringValue + " for parameter:" + key));
			}
		}
		return value;
	}
	
	abstract String getString(String key, String oldValue, boolean required ) throws OrkException;
}
