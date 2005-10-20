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

#include "ace/Singleton.h"
#include <map>
#include "StdString.h"
#include "Object.h"

/** The ObjectFactory can be used to instanciate Objects based on class name.
    All existing Objects must be registered to the ObjectFactory at startup.
*/
class DLL_IMPORT_EXPORT ObjectFactory
{
public:
	void Initialize();
	ObjectRef NewInstance(CStdString& className);

	void RegisterObject(ObjectRef&);
private:
	std::map<CStdString, ObjectRef> m_classes;
};

typedef ACE_Singleton<ObjectFactory, ACE_Thread_Mutex> ObjectFactorySingleton;

#endif

