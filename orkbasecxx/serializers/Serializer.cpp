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

#define _WINSOCKAPI_		// prevents the inclusion of winsock.h

#include "ace/OS_NS_time.h"
#include "Object.h"
#include "Serializer.h"
#include "Utils.h"

using namespace XERCES_CPP_NAMESPACE;

Serializer::Serializer(Object* object)
{
	m_object = object;
}

void Serializer::SetObject(Object* object)
{
	m_object = object;
}

void Serializer::DeSerialize()
{
	m_deSerialize = true;		// Set DeSerialize mode
	m_object->Define(this);
	m_object->Validate();
}


void Serializer::IntValue(const char* key, int& value, bool required)
{
	if (m_deSerialize == true)
	{
		GetInt(key, value, required);
	}
	else
	{
		AddInt(key, value);
	}
}

void Serializer::DoubleValue(const char* key, double& value, bool required)
{
	if (m_deSerialize == true)
	{
		GetDouble(key, value, required);
	}
	else
	{
		AddDouble(key, value);
	}
}

void Serializer::BoolValue(const char* key, bool& value, bool required)
{
	if (m_deSerialize == true)
	{
		GetBool(key, value, required);
	}
	else
	{
		AddBool(key, value);
	}
}

void Serializer::StringValue(const char* key, CStdString& value, bool required)
{
	if (m_deSerialize == true)
	{
		GetString(key, value, required);
	}
	else
	{
		AddString(key, value);
	}
}

void Serializer::EnumValue(const char* key, int& value, StringToEnumFunction toEnum, EnumToStringFunction toString, bool required)
{
	if (m_deSerialize == true)
	{
		GetEnum(key, value, toEnum, required);
	}
	else
	{
		AddEnum(key, value, toString);
	}
}

void Serializer::CsvValue(const char* key, std::list<CStdString>& value, bool required)
{
	if (m_deSerialize == true)
	{
		GetCsv(key, value, required);
	}
	else
	{
		AddCsv(key, value);
	}
}

void Serializer::CsvMapValue(const char* key, std::map<CStdString, CStdString>& value, bool required)
{
	if (m_deSerialize == true)
	{
		GetCsvMap(key, value, required);
	}
	else
	{
		AddCsvMap(key, value);
	}
}

void Serializer::DateValue(const char* key, time_t& value, bool required)
{
	if (m_deSerialize == true)
	{
		GetDate(key, value, required);
	}
	else
	{
		AddDate(key, value);
	}
}

void Serializer::IpRangesValue(const char* key, IpRanges& value, bool required)
{
	if (m_deSerialize == true)
	{
		GetIpRanges(key, value, required);
	}
	else
	{
		AddIpRanges(key, value);
	}
}



//=====================================
void Serializer::AddInt(const char* key, int value)
{
	CStdString valueString = IntToString(value);
	AddString(key, valueString);
}

void Serializer::AddDouble(const char* key, double value)
{
	CStdString valueString = DoubleToString(value);
	AddString(key, valueString);
}

void Serializer::AddBool(const char* key, bool value)
{
	if(value)
	{
		CStdString trueString("true");
		AddString(key, trueString);
	}
	else
	{
		CStdString falseString("false");
		AddString(key, falseString);
	}
}

void Serializer::AddEnum(const char* key, int value, EnumToStringFunction function)
{
	if(!function)
	{
		throw(CStdString("Serializer: wrong enumerated type conversion function for parameter:") + key);
	}
	else
	{
		CStdString valueString = function(value);
		AddString(key, valueString);
	}
}

void Serializer::AddCsv(const char* key,  std::list<CStdString>& value)
{
	CStdString csvString;
	bool first = true;
	for(std::list<CStdString>::iterator it = value.begin(); it!=value.end(); it++)
	{
		if(!first)
		{
			csvString += ",";
		}
		first = false;
		CStdString escapedValue;
		EscapeCsv(*it, escapedValue);
		csvString += escapedValue;
	}
	AddString(key, csvString);
}

void Serializer::AddCsvMap(const char* key,  std::map<CStdString, CStdString>& value)
{
	CStdString csvMapString;
	bool first = true;
	for(std::map<CStdString, CStdString>::iterator pair = value.begin(); pair!=value.end(); pair++)
	{
		if(!first)
		{
			csvMapString += ",";
		}
		first = false;

		CStdString pairKey = pair->first;
		CStdString escapedPairKey;
		EscapePair(pairKey, escapedPairKey);
		CStdString pairVal = pair->second;
		CStdString escapedPairVal;
		EscapePair(pairVal, escapedPairVal);

		CStdString csvElement = escapedPairKey + ":" + escapedPairVal;
		CStdString escapedCsvElement;
		EscapeCsv(csvElement, escapedCsvElement);
		csvMapString += escapedCsvElement;
	}
	AddString(key, csvMapString);
}

void Serializer::AddDate(const char* key, time_t value)
{
	struct tm date;
	ACE_OS::localtime_r(&value ,&date);
	int month = date.tm_mon + 1;				// january=0, decembre=11
	int year = date.tm_year + 1900;
	CStdString dateString;
	dateString.Format("%.4d-%.2d-%.2d_%.2d-%.2d-%.2d", year, month, date.tm_mday, date.tm_hour, date.tm_min, date.tm_sec);
	AddString(key, dateString);
}

void Serializer::AddIpRanges(const char* key,  IpRanges& value)
{
	;	// Not yet implemented
}

//====================================================================
void Serializer::GetInt(const char* key, int&value, bool required)
{
	CStdString stringValue;
	GetString(key, stringValue, required);
	if(!stringValue.IsEmpty())
	{
		value = StringToInt(stringValue);
	}
}

void Serializer::GetDouble(const char* key, double&value, bool required)
{
	CStdString stringValue;
	GetString(key, stringValue, required);
	if(!stringValue.IsEmpty())
	{
		value = StringToDouble(stringValue);
	}
}

void Serializer::GetBool(const char* key, bool&value, bool required)
{
	CStdString stringValue;
	GetString(key, stringValue, required);
	stringValue.ToLower();
	if(	stringValue == "true" || stringValue == "yes" || stringValue == "1" )
	{
		value = true;
	}
	else if( stringValue == "false" || stringValue == "no" || stringValue == "0" )
	{
		value = false;
	}
	else if( stringValue.IsEmpty() )
	{
		; // do not touch default value
	}
	else
	{
		throw(CStdString("Serializer: Invalid boolean value:") + stringValue + " for parameter:" + key);
	}
}

void Serializer::GetEnum(const char* key, int& value, StringToEnumFunction function, bool required)
{
	if(!function)
	{
		throw(CStdString("Serializer: missing enumerated type conversion function for parameter:") + key);
	}
	else
	{
		CStdString enumStringValue;
		GetString(key, enumStringValue, required);
		if (!enumStringValue.IsEmpty())
		{
			value = function(enumStringValue);
		}
	}

}

void Serializer::GetCsv(const char* key,  std::list<CStdString>& value, bool required)
{
	CStdString stringValue;
	stringValue.Trim();
	CStdString element;
	bool first = true;
	GetString(key, stringValue, required);
	for(unsigned int i=0; i<stringValue.length(); i++)
	{
		TCHAR c = stringValue[i];
		if(c == ',')
		{
			if(first)
			{
				first = false;	// only erase default value if something found
				value.clear();
			}
			element.Trim();
			CStdString unescapedElement;
			UnEscapeCsv(element, unescapedElement);
			value.push_back(unescapedElement);
			element.Empty();
		}
		else
		{
			element += c;
		}
	}
	if (!element.IsEmpty())
	{
		if(first)
		{
			first = false;	// only erase default value if something found
			value.clear();
		}
		element.Trim();
		CStdString unescapedElement;
		UnEscapeCsv(element, unescapedElement);
		value.push_back(unescapedElement);
	}
}

void Serializer::GetCsvMap(const char* key, std::map<CStdString, CStdString>& value, bool required)
{
	std::list<CStdString> cvsList;
	GetCsv(key, cvsList, required);

	for(std::list<CStdString>::iterator it = cvsList.begin(); it != cvsList.end(); it++)
	{
		CStdString keyValuePair = *it;
		int colonPos = keyValuePair.Find(':');
		if(colonPos != -1)
		{
			CStdString key = keyValuePair.Left(colonPos);
			CStdString val = keyValuePair.Right(keyValuePair.size() - colonPos - 1);

			CStdString unescapedKey;
			UnEscapePair(key, unescapedKey);
			CStdString unescapedVal;
			UnEscapePair(val, unescapedVal);

			value.insert(std::make_pair(unescapedKey, unescapedVal));
		}
		else
		{
			throw(CStdString("DeSerializer: GetCsvMap: missing colon in map element"));
		}
	}
}

void Serializer::GetDate(const char* key, time_t& value, bool required)
{
	throw (CStdString("DeSerializer: GetDate: not implemented yet"));

	CStdString stringValue;
	GetString(key, stringValue, required);
	if(!stringValue.IsEmpty())
	{
		//value = ;
	}
}

void Serializer::GetIpRanges(const char* key,  IpRanges& value, bool required)
{
	GetCsv(key, value.m_asciiIpRanges, required);
	value.Compute();
}


//-------------------------------------------------------------------------
// Escape the comma and percent characters for adding string to csv list
void Serializer::EscapeCsv(CStdString& in, CStdString& out)
{
	for(unsigned int i=0; i<in.length();i++)
	{
		TCHAR c = in[i];
		if (c == ',')
		{
			out+= "%c";
		}
		else if (c == '%')
		{
			out+= "%p";
		}
		else
		{
			out+= c;
		}
	}
}

// Unescape the comma and percent characters when retrieving from csv list
void Serializer::UnEscapeCsv(CStdString& in, CStdString& out)
{
	unsigned int iin = 0;

	while(iin<in.length())
	{ 
		if ( in[iin] == '%')
		{
			iin++;

			switch (in[iin])
			{
			case 'c':
				out += ',';
				break;
			case 'p':
				out += '%';
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

//--------------------------------------------------------------------------------------------
// Escape the colon and percent characters for adding string to a pair of the form "key:value"
void Serializer::EscapePair(CStdString& in, CStdString& out)
{
	for(unsigned int i=0; i<in.length();i++)
	{
		TCHAR c = in[i];
		if (c == ':')
		{
			out+= "%k";
		}
		else if (c == '%')
		{
			out+= "%p";
		}
		else
		{
			out+= c;
		}
	}
}

// UnEscape the colon and percent characters after retrieving a key or value from a pair of the form "key:value"
void Serializer::UnEscapePair(CStdString& in, CStdString& out)
{
	unsigned int iin = 0;

	while(iin<in.length())
	{ 
		if ( in[iin] == '%')
		{
			iin++;

			switch (in[iin])
			{
			case 'k':
				out += ':';
				break;
			case 'p':
				out += '%';
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

//==========================================================

void KeyValueSerializer::GetString(const char* key, CStdString& value, bool required)
{
	std::map<CStdString, CStdString>::iterator it = m_map.find(CStdString(key));
	if (it != m_map.end())
	{
		value = it->second;
	}
	else if (required == true)
	{
		throw CStdString(CStdString("Serializer::GetString: required parameter missing:") + key);
	}
}

void KeyValueSerializer::ObjectValue(const char* key, Object& value, bool required)
{
	throw CStdString(CStdString("KeyValueSerializer::ObjectValue: Nested objects not allowed for key-value serializers"));
}

void KeyValueSerializer::ListValue(const char* key, std::list<ObjectRef>& value, Object& model, bool required )
{
	throw CStdString(CStdString("KeyValueSerializer::ListValue: Nested objects not allowed for key-value serializers"));
}

