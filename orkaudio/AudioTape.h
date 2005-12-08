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
#include "messages/Message.h"

class AudioTapeDescription : public Object
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
};

class AudioTape
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

#define FF_NATIVE "native"
#define FF_GSM "GSM"
#define FF_ULAW "ulaw"
#define FF_ALAW "alaw"
#define FF_PCMWAV "pcmwav"
#define FF_UNKNOWN "unknown"
	typedef enum
	{
		FfUnknown = 0,
		FfNative = 1,
		FfGsm = 2,
		FfUlaw = 3,
		FfAlaw = 4,
		FfPcmWav = 5,
		FfInvalid = 6
	} FileFormatEnum;
	static int FileFormatToEnum(CStdString& format);
	static CStdString FileFormatToString(int formatEnum);
	static CStdString GetFileFormatExtension(FileFormatEnum);


	AudioTape(CStdString &portId);

	void AddAudioChunk(AudioChunkRef chunkRef, bool remote = false);
	void Write();
	void SetShouldStop();
	bool IsStoppedAndValid();
	void AddCaptureEvent(CaptureEventRef eventRef, bool send = true);
	void GetMessage(MessageRef& msg);
	/** Returns an identifier for the tape which corresponds to the filename without extension */
	CStdString GetIdentifier();
	CStdString GetPath();
	AudioFileRef GetAudioFileRef();

	CStdString m_portId;
	CStdString m_localParty;
	CStdString m_localEntryPoint;
	CStdString m_remoteParty;
	CaptureEvent::DirectionEnum m_direction;
	time_t m_beginDate;
	time_t m_endDate;
	time_t m_duration;
private:
	void GenerateFilePathAndIdentifier();

	CStdString m_filePath;
	CStdString m_fileIdentifier;
	CStdString m_fileExtension;	//Corresponds to the extension the tape will have after compression

	std::queue<AudioChunkRef> m_chunkQueue;
	std::queue<AudioChunkRef> m_remoteChunkQueue;	// used if stereo capture

	std::queue<CaptureEventRef> m_eventQueue;
	std::queue<CaptureEventRef> m_toSendEventQueue;

	AudioFileRef m_audioFileRef;
	ACE_Thread_Mutex m_mutex;
	StateEnum m_state;
	bool m_shouldStop;
};

typedef boost::shared_ptr<AudioTape> AudioTapeRef;

#endif

