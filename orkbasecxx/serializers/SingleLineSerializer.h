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

#ifndef __SINGLELINESERIALIZER_H__
#define __SINGLELINESERIALIZER_H__

#include "messages/Message.h"
#include "serializers/Serializer.h"

/** Serializer that generates and parses objects to and from a single line of text.
    key-value pairs are separated by spaces.
    key and values are separated by equal signs.
    example: message=doit what=run
*/
class DLL_IMPORT_EXPORT_ORKBASE SingleLineSerializer : public KeyValueSerializer
{
public:
	SingleLineSerializer(Object* object) : KeyValueSerializer(object){};

	void AddString(const char* key, CStdString& value);
	CStdString Serialize();

	void DeSerialize(CStdString& input);

	void EscapeSingleLine(CStdString& in, CStdString& out);
	void UnEscapeSingleLine(CStdString& in, CStdString& out);

	static CStdString FindClass(CStdString& input);
private:
	CStdString m_output;
};

#endif

