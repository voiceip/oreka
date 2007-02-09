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
#pragma warning( disable: 4786 )

#define _WINSOCKAPI_		// prevents the inclusion of winsock.h

#include "Utils.h"
#include "ObjectFactory.h"

ObjectFactory* ObjectFactory::m_singleton = NULL;

ObjectFactory::ObjectFactory()
{
}

void ObjectFactory::Initialize()
{
	m_singleton = new ObjectFactory();
}

ObjectFactory* ObjectFactory::GetSingleton()
{
	return m_singleton;
}


ObjectRef ObjectFactory::NewInstance(CStdString& className)
{
	MutexSentinel mutexSentinel(m_mutex);
	std::map<CStdString, ObjectRef>::iterator pair;
	pair = m_classes.find(className);

	if (pair == m_classes.end())
	{
		return ObjectRef();		// Empty
	}
	else
	{
		ObjectRef ref = pair->second;
		return ref->NewInstance();
	}
}

void ObjectFactory::RegisterObject(ObjectRef& objRef)
{
	MutexSentinel mutexSentinel(m_mutex);
	std::map<CStdString, ObjectRef>::iterator pair = m_classes.find(objRef->GetClassName());
	if(pair != m_classes.end())
	{
		m_classes.erase(pair);
	}
	m_classes.insert(std::make_pair(objRef->GetClassName(), objRef));
}


