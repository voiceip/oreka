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

#include "ObjectFactory.h"


void ObjectFactory::Initialize()
{
	;
}

ObjectRef ObjectFactory::NewInstance(CStdString& className)
{
	std::map<CStdString, ObjectRef>::iterator pair;
	pair = m_classes.find(className);

	if (pair == m_classes.end())
	{
		return ObjectRef();		// Empty
	}
	else
	{
		return pair->second;
	}
}

void ObjectFactory::RegisterObject(ObjectRef& objRef)
{
	m_classes.insert(std::make_pair(objRef->GetClassName(), objRef));
}


