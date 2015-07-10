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
#include "shared_ptr.h"
#include <queue>
#include <vector>
#include "AudioCapture.h"
#include "audiofile/AudioFile.h"
#include "messages/TapeMsg.h"

class AudioDirectionMarks
{
public:
	AudioDirectionMarks();

	time_t m_timestamp;
	time_t m_nextTimestampMark;
	CaptureEvent::AudioKeepDirectionEnum m_audioKeepDirectionEnum;
};
typedef oreka::shared_ptr<AudioDirectionMarks> AudioDirectionMarksRef;

class DLL_IMPORT_EXPORT_ORKBASE MediaType
{
public:
#define UNKNOWN_TYPE "unknown"
#define AUDIO_TYPE "audio"
#define VIDEO_TYPE "video"
#define INSTANT_MSG "im"
	MediaType();
	typedef enum{
		UnKnownType = 0,
		AudioType = 1,
		VideoType = 2,
		InstantMessageType = 3
	} MediaTypeEnum;
	MediaTypeEnum m_type;
	static int MediaTypeToEnum(CStdString mediaType);
	static CStdString MediaTypeToString(int mediaType);
};

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
        CaptureEvent::LocalSideEnum m_localSide;
        CaptureEvent::AudioKeepDirectionEnum m_audioKeepDirectionEnum;
	time_t m_beginDate;
	int m_duration;
	CStdString m_localIp;
	CStdString m_remoteIp;
	CStdString m_filename;
	bool m_onDemand;
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
	void SetExtension(CStdString& ext);
	void SetOrkUid(CStdString& orkuid);
	AudioFileRef GetAudioFileRef();
	bool IsReadyForBatchProcessing();
	void GetDetails(TapeMsg* msg);
	void PopulateTag(CStdString key, CStdString value);

	std::vector<AudioDirectionMarksRef> m_audioDirectionMarks;
	CStdString m_portId;
	CStdString m_localParty;
	CStdString m_localEntryPoint;
	CStdString m_remoteParty;
	CaptureEvent::DirectionEnum m_direction;
	CaptureEvent::LocalSideEnum m_localSide;
	CaptureEvent::AudioKeepDirectionEnum m_audioKeepDirectionEnum;
	time_t m_beginDate;
	time_t m_endDate;
	time_t m_duration;
	CStdString m_localIp;
	CStdString m_remoteIp;
	CStdString m_trackingId;
	CStdString m_nativeCallId;
	StateEnum m_state;
	int m_numErrors;
	bool m_onDemand;
	bool m_keep;
	bool m_noAudio;

	bool m_isExternal;		// set if the tape is imported manually from API
	MediaType::MediaTypeEnum m_mediaType;		// media type of manually imported tape
	bool m_isSuccessfulImported;
	bool m_isDoneProcessed;
	CStdString m_externalFileName;		//name of imported file, its not conventional orkaudio name, its convetional orkaudio identifier + orginal name

	TapeResponseRef m_tapeResponse;

	/*
	 * This function has been made public in order to allow the TapeFileNaming
	 * tape processor to do its work in the easiest way, with as little as possible
	 * access to the internal variables of the AudioTape class.
	 */
	void GenerateFinalFilePathAndIdentifier();

private:
	void GenerateCaptureFilePathAndIdentifier();
	void GenerateFinalFilePath();
	void PreventFileIdentifierCollision(CStdString& path, CStdString& identifier, CStdString& extension);
	void PopulateTapeMessage(TapeMsg* msg, CaptureEvent::EventTypeEnum eventType);

	CStdString m_filePath;
	CStdString m_fileIdentifier;
	CStdString m_fileExtension;	//Corresponds to the extension the tape will have after compression

	CStdString m_year;
	CStdString m_day;
	CStdString m_month;
	CStdString m_hour;
	CStdString m_min;
	CStdString m_sec;

	std::queue<AudioChunkRef> m_chunkQueue;
	int m_pushCount;
	int m_popCount;
	int m_highMark;
	unsigned int m_chunkQueueDataSize;
	bool m_chunkQueueErrorReported;

	std::queue<CaptureEventRef> m_eventQueue;
	std::queue<CaptureEventRef> m_toSendEventQueue;

	AudioFileRef m_audioFileRef;
	ACE_Thread_Mutex m_mutex;
	bool m_shouldStop;
	bool m_readyForBatchProcessing;
	CStdString m_orkUid;

	std::map<CStdString, CStdString> m_tags;

	int m_bytesWritten;
	time_t m_lastLogWarning;

	bool m_passedPartyFilterTest;
};

typedef oreka::shared_ptr<AudioTape> AudioTapeRef;

//==========================================================

class DLL_IMPORT_EXPORT_ORKBASE TapeAttributes
{
public:
#define TA_NATIVECALLID "[nativecallid]"
#define TA_TRACKINGID "[trackingid]"
#define TA_DIRECTION "[direction]"
#define TA_SHORTDIRECTION "[shortdirection]"
#define TA_REMOTEPARTY "[remoteparty]"
#define TA_LOCALPARTY "[localparty]"
#define TA_LOCALENTRYPOINT "[localentrypoint]"
#define TA_LOCALIP "[localip]"
#define TA_REMOTEIP "[remoteip]"
#define TA_HOSTNAME "[hostname]"
#define TA_YEAR "[year]"
#define TA_DAY "[day]"
#define TA_MONTH "[month]"
#define TA_HOUR "[hour]"
#define TA_MIN "[min]"
#define TA_SEC "[sec]"

	typedef enum {
		TaUnknown = 0,
		TaNativeCallId = 1,
		TaTrackingId = 2,
		TaDirection = 3,
		TaShortDirection = 4,
		TaRemoteParty = 5,
		TaLocalParty = 6,
		TaLocalEntryPoint = 7,
		TaLocalIp = 8,
		TaRemoteIp = 9,
		TaHostname = 10,
		TaYear = 11,
		TaDay = 12,
		TaMonth = 13,
		TaHour = 14,
		TaMin = 15,
		TaSec = 16
	} TapeAttributeEnum;

	static int TapeAttributeToEnum(CStdString& ta);
	static CStdString TapeAttributeToString(int ta);
};

#endif

