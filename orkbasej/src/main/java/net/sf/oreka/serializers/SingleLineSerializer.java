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

import net.sf.oreka.OrkException;
import net.sf.oreka.OrkObject;
import net.sf.oreka.OrkObjectFactory;

/** 
 * Serializer that generates and parses objects to and from a single line of text.
 * key-value pairs are separated by spaces.
 * key and values are separated by equal signs.
 * example: message=doit what=run 
 */
public class SingleLineSerializer extends KeyValueSerializer
{
	StringBuffer output;
	
	public String serialize(OrkObject input) throws OrkException
	{
		output = new StringBuffer();
		deserialize = false;		// Set Serialize mode
		addClassName(input.getOrkClassName());
		input.define(this);
		return output.toString();
	}
	
	enum SingleLineState {StartState, KeyState, ValueState};
	
	public OrkObject deSerialize(String input) throws OrkException
	{
		// read string and extract values into map
		input = input.trim();

		SingleLineState state = SingleLineState.StartState;

		StringBuffer key = new StringBuffer();
		StringBuffer value = new StringBuffer();
		String firstValue = null;
		
		for(int i=0; i<input.length(); i++)
		{
			Character character = input.charAt(i);

			switch(state)
			{
			case StartState:
				if(character == ' ')
				{
					;	// ignore spaces
				}
				
				else if ( Character.isLetterOrDigit(character) )
				{
					state = SingleLineState.KeyState;
					key.append(character);
				}
				else
				{
					throw new OrkException("SingleLineSerializer.deSerialize: Cannot find key start, keys must be alphanum, wrong char:"  + character + " input:" + input);
				}
				break;
				
			case KeyState:
				if( Character.isLetterOrDigit(character) )
				{
					key.append(character);
				}
				else if (character == '=')
				{
					state = SingleLineState.ValueState;
					value = new StringBuffer();
				}
				else
				{
					throw new OrkException("SingleLineSerializer.deSerialize: Invalid key character, keys must be alphanum, wrong char:"  + character + " input:" + input);
				}
				break;
			case ValueState:
				if( character == '=')
				{
					throw new OrkException("SingleLineSerializer.deSerialize: equals sign misplaced at index:" + i + " in input:" + input);				
				}
				else if (character == ' ')
				{
					state = SingleLineState.StartState;
				}
				else
				{
					value.append(character);
				}
				break;
			}	// switch(state)

			if ( (state == SingleLineState.StartState) || (i == (input.length()-1)) )
			{
				if (key.length() > 0)
				{
					// Add pair to key-value map
					map.put(key.toString().toLowerCase(), unEscapeSingleLine(value.toString()));
					if (firstValue == null)
					{
						firstValue = value.toString().toLowerCase();
					}
					key = new StringBuffer();
					value = new StringBuffer();
				}
			}
		}

		deserialize = true;		// Set DeSerialize mode
		
		OrkObject obj = OrkObjectFactory.instance().newOrkObject(firstValue);
		obj.define(this);
		obj.validate();
		return obj;
	}
	
	public void addClassName(String value)
	{
		addString(classKey, value);
	}
	
	void addString(String key, String value)
	{
		String pair;
		String escapedValue = escapeSingleLine(value);
		pair = key + "=" + escapedValue + " ";
		output.append(pair);
	}

	/**
	 * Finds the Object Class from a serialized string. 
	 * i.e. for a serialized object of the form:
	 * 		message=dosomething when=now
	 * it returns "dosomething"
	 * 
	 * @param input
	 * @return
	 */
	/*
	static String findClass(String input)
	{
		String result = "";
		int equalsPosition = input.indexOf("=");
		if (equalsPosition != -1)
		{
			int spacePostion = input.indexOf(" ");
			if (spacePostion > equalsPosition)
			{
				result = input.substring(equalsPosition+1, spacePostion-1);
			}
			else
			{
				result = input.substring(equalsPosition+1, input.length() - 1);
			}
		}
		return result.toLowerCase();
	}
	*/
	
	String escapeSingleLine(String in)
	{		
		StringBuffer out = new StringBuffer();
		for(int i=0; i<in.length();i++)
		{
			char c = in.charAt(i);
			if (c == ' ')
			{
				out.append("%s");
			}
			else if (c == ':')
			{
				out.append("%c");
			}
			else if (c == '%')
			{
				out.append("%p");
			}
			else
			{
				out.append(c);
			}
		}		
		return out.toString();
	}
	
	String unEscapeSingleLine(String in)
	{
		StringBuffer out = new StringBuffer();
		int iin = 0;

		while(iin<in.length())
		{ 
			if ( in.charAt(iin) == '%')
			{
				iin++;

				switch (in.charAt(iin))
				{
				case 's':
					out.append(' ');
					break;
				case 'c':
					out.append(':');
					break;
				case 'p':
					out.append('%');
					break;
				}
			}
			else
			{
				out.append(in.charAt(iin));
			}
			iin++;
		}
		return out.toString();
	}
}
