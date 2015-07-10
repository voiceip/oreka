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

#ifndef __DIRECTIONSELECTOR_H__
#define __DIRECTIONSELECTOR_H__

#include "ThreadSafeQueue.h"
#include "TapeProcessor.h"
#include "AudioTape.h"
#include "ace/Thread_Mutex.h"
#include <map>

class  DirectionSelector;
typedef oreka::shared_ptr<DirectionSelector> DirectionSelectorRef;

#define LOCAL_AREA_CODES_MAP_FILE "area-codes-recorded-side.csv"
#define ETC_LOCAL_AREA_CODES_MAP_FILE "/etc/orkaudio/area-codes-recorded-side.csv"

/**
 * This tape processor handles the audio filtering
 */
class DLL_IMPORT_EXPORT_ORKBASE DirectionSelector : public TapeProcessor
{
public:
	static void Initialize();

	CStdString __CDECL__ GetName();
	TapeProcessorRef __CDECL__ Instanciate();
	void __CDECL__ AddAudioTape(AudioTapeRef& audioTapeRef);
	static void ThreadHandler(void *args);
	void SetQueueSize(int size);

private:
	DirectionSelector();
	static TapeProcessorRef m_singleton;
	ThreadSafeQueue<AudioTapeRef> m_audioTapeQueue;

	void ProcessAreaCodesMap(char *line, int ln);
	void LoadAreaCodesMap();

	size_t m_threadCount;
	ACE_Thread_Mutex m_mutex;
	int m_currentDay;
	std::map<CStdString, CStdString> m_areaCodesMap;
};

#endif

