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

#include "serializers/SingleLineSerializer.h"
#include "serializers/DomSerializer.h"
#include "serializers/UrlSerializer.h"
#include "Object.h"


CStdString Object::SerializeSingleLine()
{
	SingleLineSerializer serializer(this);
	return serializer.Serialize();
}

void Object::DeSerializeSingleLine(CStdString& input)
{
	SingleLineSerializer serializer(this);
	serializer.DeSerialize(input);
}

void Object::SerializeDom(XERCES_CPP_NAMESPACE::DOMDocument* doc)
{
	DomSerializer serializer(this);
	serializer.Serialize(doc);
}

void Object::DeSerializeDom(DOMNode* doc)
{
	DomSerializer serializer(this);
	serializer.DeSerialize(doc);
}

CStdString Object::SerializeUrl()
{
	UrlSerializer serializer(this);
	return serializer.Serialize();
}

void Object::DeSerializeUrl(CStdString& input)
{
	UrlSerializer serializer(this);
	serializer.DeSerialize(input);
}
