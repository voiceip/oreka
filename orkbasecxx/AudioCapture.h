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

#ifndef __AUDIOCAPTURE_H__
#define __AUDIOCAPTURE_H__

#include "time.h"
#include "StdString.h"
#include "dll.h"
#include "OrkBase.h"

#include "shared_ptr.h"


typedef enum
{
	UnknownAudio = 0,
	PcmAudio = 1,
	AlawAudio = 2,
	UlawAudio = 3,
	GsmAudio = 4,
	IlbcAudio = 5,
	G722Audio = 6,
	G721Audio = 7,
	InvalidAudio = 8
} AudioEncodingEnum;

/** 
 * Serialization friendly details struct
 */
#define MEDIA_CHUNK_MARKER 0x2A2A2A2A // corresponds to "****"
#define MEDIA_CHUNK_EOS_MARKER 0x454F534D	/// corresponds to "EOSM"

#define RTP_PAYLOAD_TYPE_MAX 127

class DLL_IMPORT_EXPORT_ORKBASE AudioChunkDetails
{
public:
	AudioChunkDetails();
	void Clear();

	int m_marker;
	AudioEncodingEnum m_encoding;
	unsigned int m_numBytes;
	unsigned int m_timestamp;			// usually relative timestamp measured in samples
	unsigned int m_arrivalTimestamp;	// usually unix timestamp of arrival
	unsigned int m_sequenceNumber;
	unsigned int m_sampleRate;
	char m_rtpPayloadType;				// -1 if none
	unsigned char m_channel;			// 0 if mono, 1 or 2 if stereo, 100 if we have
							// separated multiple channels
};

/** 
 * This class represents a piece of audio. 
 */
class DLL_IMPORT_EXPORT_ORKBASE AudioChunk
{
public: 
	AudioChunk();
	AudioChunk(int numChannels);
	~AudioChunk();

	void ToString(CStdString&);

	/** Creates n zeroed buffers where n=m_numChannels */
	void CreateMultiChannelBuffers(AudioChunkDetails& details);

	/** Allocate a new empty buffer (zeroed) */
	void* CreateBuffer(AudioChunkDetails& details);

	/** Copy external buffer to internal buffer. Create internal buffer if necessary */
	void SetBuffer(void* pBuffer, AudioChunkDetails& details);

	/** Copy external buffer to internal buffer. Create internal buffer if necessary */
	void SetBuffer(void* pBuffer, AudioChunkDetails& details, int chan);

	/** Computes the Root-Mean-Square power value of the buffer */
	double ComputeRms();
	/** Compute the RMS decibel value of the buffer with a 0 dB reference being the maximum theoretical RMS power of the buffer (2^15) */
	double ComputeRmsDb();

	/** Free's all memory allocated */
	void FreeAll();

	int GetNumSamples();
	int GetNumBytes();
	int GetSampleRate();
	double GetDurationSec();
	AudioEncodingEnum GetEncoding();
	AudioChunkDetails* GetDetails();
	void SetDetails(AudioChunkDetails* details);

	void * m_pBuffer;

	//==========================================================
	// Additions to support separate audio for multiple channels

	int m_numChannels;
	void ** m_pChannelAudio;

private:
	AudioChunkDetails m_details;
};

typedef oreka::shared_ptr<AudioChunk> AudioChunkRef;


//==========================================================

class DLL_IMPORT_EXPORT_ORKBASE CaptureEvent
{
public:
	CaptureEvent();

#define LOCALSIDE_UNKN "unknown"
#define LOCALSIDE_SIDE1 "side1"
#define LOCALSIDE_SIDE2 "side2"
#define LOCALSIDE_BOTH "both"
#define LOCALSIDE_INVALID "invalid"
	typedef enum {
		LocalSideUnkn = 0,
		LocalSideSide1 = 1,
		LocalSideSide2 = 2,
		LocalSideBoth = 3,
		LocalSideInvalid = 4
	} LocalSideEnum;
	static CStdString LocalSideToString(int);
	static int LocalSideToEnum(CStdString& localSideString);

#define AUDIOKEEPDIRECTION_BOTH "both"
#define AUDIOKEEPDIRECTION_LOCAL "local"
#define AUDIOKEEPDIRECTION_REMOTE "remote"
#define AUDIOKEEPDIRECTION_NONE "none"
#define AUDIOKEEPDIRECTION_INVALID "invalid"
	typedef enum {
		AudioKeepDirectionBoth = 0,
		AudioKeepDirectionLocal = 1,
		AudioKeepDirectionRemote = 2,
		AudioKeepDirectionNone = 3,
		AudioKeepDirectionInvalid = 4
	} AudioKeepDirectionEnum;
	static CStdString AudioKeepDirectionToString(int);
	static int AudioKeepDirectionToEnum(CStdString& audioKeepDirectionString);
	static bool AudioKeepDirectionIsDefault(CStdString& audioKeepDirectionString);

#define DIR_IN "in"
#define DIR_OUT "out"
#define DIR_UNKN "unkn"
#define DIR_IN_SHORT "I"
#define DIR_OUT_SHORT "O"
#define DIR_UNKN_SHORT "U"
	typedef enum {	
		DirIn = 0, 
		DirOut = 1, 
		DirUnkn = 2
	} DirectionEnum;
	static CStdString DirectionToString(int);
	static CStdString DirectionToShortString(int direction);
	static int DirectionToEnum(CStdString& dir);

#define ET_UNKNOWN "unknown"
#define ET_START "start"
#define ET_STOP "stop"
#define ET_DIRECTION "direction"
#define ET_REMOTEPARTY "remoteparty"
#define ET_LOCALPARTY "localparty"
#define ET_LOCALENTRYPOINT "localentrypoint"
#define ET_KEYVALUE "keyvalue"
#define ET_LOCALIP "localip"
#define ET_REMOTEIP "remoteip"
#define ET_LOCALMAC "localmac"
#define ET_REMOTEMAC "remotemac"
#define ET_ORKUID "orkuid"
#define ET_ENDMETADATA "endmetadata"
#define ET_READY "ready"
#define ET_UPDATE "update"
#define ET_CALLID "callid"
#define ET_INVALID "invalid"
#define ET_LOCALSIDE "localside"
#define ET_AUDIOKEEPDIRECTION "audiokeepdirection"
	typedef enum
	{
		EtUnknown = 0,
		EtStart = 1,
		EtStop = 2,
		EtDirection = 3,
		EtRemoteParty = 4,
		EtLocalParty = 5,
		EtLocalEntryPoint = 6,
		EtKeyValue = 7,
		EtLocalIp = 8,
		EtRemoteIp = 9,
		EtLocalMac = 10,
		EtRemoteMac = 11,
		EtOrkUid = 12,
		EtEndMetadata = 13,
		EtReady = 14,
		EtUpdate = 15,
		EtCallId = 16,
                EtLocalSide = 17,
                EtAudioKeepDirection = 18,
		EtInvalid = 19
	} EventTypeEnum;
	static CStdString EventTypeToString(int eventTypeEnum);
	static int EventTypeToEnum(CStdString&);
	
	time_t m_timestamp;
	int m_offsetMs;
	EventTypeEnum m_type;
	CStdString m_key;
	CStdString m_value;
};

typedef oreka::shared_ptr<CaptureEvent> CaptureEventRef;


//=========================================================
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

int DLL_IMPORT_EXPORT_ORKBASE FileFormatToEnum(CStdString& format);
CStdString DLL_IMPORT_EXPORT_ORKBASE FileFormatToString(int formatEnum);
CStdString DLL_IMPORT_EXPORT_ORKBASE FileFormatGetExtension(FileFormatEnum);

#endif



