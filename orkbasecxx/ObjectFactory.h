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

#ifndef __OBJECTFACTORY_H__
#define __OBJECTFACTORY_H__

#include "ace/Thread_Mutex.h"
#include "ace/Singleton.h"
#include <map>
#include "StdString.h"
#include "Object.h"

/** The ObjectFactory can be used to instanciate Objects based on class name.
    All existing Objects must be registered to the ObjectFactory at startup.
*/
class ObjectFactory;

class DLL_IMPORT_EXPORT_ORKBASE ObjectFactory
{
public:
	static void Initialize();
	static ObjectFactory* GetSingleton();

	ObjectRef NewInstance(CStdString& className);

	void RegisterObject(ObjectRef&);
private:
	ObjectFactory();
	static ObjectFactory* m_singleton;
	std::map<CStdString, ObjectRef> m_classes;
	ACE_Thread_Mutex m_mutex;
};

#endif

