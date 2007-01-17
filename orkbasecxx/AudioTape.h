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

#ifndef __AUDIOTAPE_H__
#define __AUDIOTAPE_H__

#include "ace/Thread_Mutex.h"
#include "time.h"
#include "StdString.h"
#include "boost/shared_ptr.hpp"
#include <queue>
#include "AudioCapture.h"
#include "audiofile/AudioFile.h"
#include "messages/TapeMsg.h"


class DLL_IMPORT_EXPORT_ORKBASE AudioTapeDescription : public Object
{
public:
	AudioTapeDescription();
	void Define(Serializer* s);
	void Validate();

	CStdString GetClassName();
	ObjectRef NewInstance();

	ObjectRef Process();

	CStdString m_capturePort;
	CStdString m_localParty;
	CStdString m_localEntryPoint;
	CStdString m_remoteParty;
	CaptureEvent::DirectionEnum m_direction;
	time_t m_beginDate;
	int m_duration;
	CStdString m_localIp;
	CStdString m_remoteIp;
};

class DLL_IMPORT_EXPORT_ORKBASE AudioTape
{
public:
	typedef enum 
	{
		StateUnknown = 0,
		StateCreated = 1,
		StateActive = 2,
		StateStopped = 3,
		StateError = 4,
		StateInvalid = 5
	} StateEnum; 

	AudioTape(CStdString& portId);
	AudioTape(CStdString& portId, CStdString& file);

	void AddAudioChunk(AudioChunkRef chunkRef);
	void Write();
	void SetShouldStop();
	bool IsStoppedAndValid();
	void AddCaptureEvent(CaptureEventRef eventRef, bool send = true);
	void GetMessage(MessageRef& msg);
	/// Returns an identifier for the tape which corresponds to the filename without extension
	CStdString GetIdentifier();
	/// Returns the full filename (including relative path) to the post-compression audio file
	CStdString GetFilename();
	CStdString GetPath();
	CStdString GetExtension();
	AudioFileRef GetAudioFileRef();
	bool IsReadyForBatchProcessing();


	CStdString m_portId;
	CStdString m_localParty;
	CStdString m_localEntryPoint;
	CStdString m_remoteParty;
	CaptureEvent::DirectionEnum m_direction;
	time_t m_beginDate;
	time_t m_endDate;
	time_t m_duration;
	CStdString m_localIp;
	CStdString m_remoteIp;
	CStdString m_trackingId;
	CStdString m_nativeCallId;

	TapeResponseRef m_tapeResponse;

private:
	void GenerateCaptureFilePathAndIdentifier();
	void GenerateFinalFilePathAndIdentifier();
	void PreventFileIdentifierCollision(CStdString& path, CStdString& identifier, CStdString& extension);

	CStdString m_filePath;
	CStdString m_fileIdentifier;
	CStdString m_fileExtension;	//Corresponds to the extension the tape will have after compression

	std::queue<AudioChunkRef> m_chunkQueue;

	std::queue<CaptureEventRef> m_eventQueue;
	std::queue<CaptureEventRef> m_toSendEventQueue;

	AudioFileRef m_audioFileRef;
	ACE_Thread_Mutex m_mutex;
	StateEnum m_state;
	bool m_shouldStop;
	bool m_readyForBatchProcessing;
	CStdString m_orkUid;
};

typedef boost::shared_ptr<AudioTape> AudioTapeRef;

#endif

