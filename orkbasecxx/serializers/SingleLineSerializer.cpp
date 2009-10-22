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

#include "ace/OS_NS_ctype.h"
#include "Utils.h"
#include "SingleLineSerializer.h"


void SingleLineSerializer::AddString(const char* key, CStdString& value)
{
	CStdString pair;
	CStdString escapedValue;
	EscapeSingleLine(value, escapedValue);
	pair.Format("%s=%s ", key, (PCSTR)escapedValue);
	m_output += pair;
}

CStdString SingleLineSerializer::Serialize()
{
	m_deSerialize = false;		// Set Serialize mode
	m_object->Define(this);
	return m_output;
}

typedef enum {SingleLineStartState, SingleLineKeyState, SingleLineValueState, SingleLineErrorState} SingleLineState;

void SingleLineSerializer::DeSerialize(CStdString& input)
{
	// read string and extract values into map
	SingleLineState state = SingleLineStartState;
	CStdString key;
	CStdString value;
	// #### Additionally, errorDescription should be converted to Exceptions
	CStdString errorDescription;

	input.Trim();

	for(unsigned int i=0; i<input.length() && state!= SingleLineErrorState; i++)
	{
		TCHAR character = input[i];

		switch(state)
		{
		case SingleLineStartState:
			if(character == ' ')
			{
				;	// ignore spaces
			}
			else if ( ACE_OS::ace_isalnum(character) )
			{
				state = SingleLineKeyState;
				key = character;
			}
			else
			{
				state = SingleLineErrorState;
				errorDescription = "Cannot find key start, keys must be alphanum";
			}
			break;
		case SingleLineKeyState:
			if( ACE_OS::ace_isalnum(character) )
			{
				key += character;
			}
			else if (character == '=')
			{
				state = SingleLineValueState;
				value.Empty();
			}
			else
			{
				state = SingleLineErrorState;
				errorDescription = "Invalid key character, keys must be alphanum";
			}
			break;
		case SingleLineValueState:
			if( character == '=')
			{
				state = SingleLineErrorState;
				errorDescription = "Value followed by = sign, value should always be followed by space sign";				
			}
			else if (character == ' ')
			{
				state = SingleLineStartState;
			}
			else
			{
				value += character;
			}
			break;
		default:
			state = SingleLineErrorState;
			errorDescription = "Non-existing state";
		}	// switch(state)

		if ( (state == SingleLineStartState) || (i == (input.length()-1)) )
		{
			if (!key.IsEmpty())
			{
				// SingleLine unescape
				CStdString unescapedValue;
				UnEscapeSingleLine(value, unescapedValue);

				// Add pair to key-value map
				m_map.insert(std::make_pair(key, unescapedValue));
				key.Empty();
				value.Empty();
			}
		}
	}	// for(int i=0; i<input.length() && state!= SingleLineErrorState; i++)

	Serializer::DeSerialize();
}

// Escape the space, equals and percent characters for serializing to Key-Value-Pair text
void SingleLineSerializer::EscapeSingleLine(CStdString& in, CStdString& out)
{
	for(unsigned int i=0; i<in.length();i++)
	{
		TCHAR c = in[i];
		if (c == ' ')
		{
			out+= "%s";
		}
		else if (c == '%')
		{
			out+= "%p";
		}
		else if (c == '=')
		{
			out+= "%e";
		}
		else
		{
			out+= c;
		}
	}
}

// Unescape the space, equals and percent characters for serializing to Key-Value-Pair text
void SingleLineSerializer::UnEscapeSingleLine(CStdString& in, CStdString& out)
{
	unsigned int iin = 0;

	while(iin<in.length())
	{ 
		if ( in[iin] == '%')
		{
			iin++;

			switch (in[iin])
			{
			case 's':
				out += ' ';
				break;
			case 'p':
				out += '%';
				break;
			case 'e':
				out += '=';
				break;
			}
		}
		else
		{
			out += in[iin];
		}
		iin++;
	}
}

CStdString SingleLineSerializer::FindClass(CStdString& input)
{
	CStdString result;
	int equalsPostion = input.Find('=');
	if (equalsPostion != -1)
	{
		int spacePostion = input.Find(' ');
		if (spacePostion > equalsPostion)
		{
			result = input.Mid(equalsPostion+1, spacePostion-equalsPostion-1);
		}
		else
		{
			result = input.Mid(equalsPostion+1, input.GetLength() - equalsPostion - 1);
		}
	}
	result.ToLower();
	return result;
}

