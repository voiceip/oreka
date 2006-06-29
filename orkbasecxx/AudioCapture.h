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

#include "boost/shared_ptr.hpp"


typedef enum
{
	UnknownAudio = 0,
	PcmAudio = 1,
	AlawAudio = 2,
	UlawAudio = 3,
	InvalidAudio = 4
} AudioEncodingEnum;

/** 
 * Serialization friendly details struct
 */
#define MEDIA_CHUNK_MARKER 0x2A2A2A2A // corresponds to "****"

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
	unsigned char m_channel;			// 0 if mono, 1 or 2 if stereo
};

/** 
 * This class represents a piece of audio. 
 */
class DLL_IMPORT_EXPORT_ORKBASE AudioChunk
{
public: 
	AudioChunk();
	~AudioChunk();

	/** Allocate a new empty buffer (zeroed) */
	void* CreateBuffer(size_t numBytes, AudioChunkDetails& details);

	/** Copy external buffer to internal buffer. Create internal buffer if necessary */
	void SetBuffer(void* pBuffer, size_t numBytes, AudioChunkDetails& details);

	/** Computes the Root-Mean-Square power value of the buffer */
	double ComputeRms();
	/** Compute the RMS decibel value of the buffer with a 0 dB reference being the maximum theoretical RMS power of the buffer (2^15) */
	double ComputeRmsDb();

	int GetNumSamples();
	int GetNumBytes();
	int GetSampleRate();
	double GetDurationSec();
	AudioEncodingEnum GetEncoding();
	AudioChunkDetails* GetDetails();
	void SetDetails(AudioChunkDetails* details);

	void * m_pBuffer;

private:
	AudioChunkDetails m_details;
};

typedef boost::shared_ptr<AudioChunk> AudioChunkRef;


//==========================================================

class DLL_IMPORT_EXPORT_ORKBASE CaptureEvent
{
public:
	CaptureEvent();

#define DIR_IN "in"
#define DIR_OUT "out"
#define DIR_UNKN "unkn"
	typedef enum {	
		DirIn = 0, 
		DirOut = 1, 
		DirUnkn = 2
	} DirectionEnum;
	static CStdString DirectionToString(int);
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
#define ET_INVALID "invalid"
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
		EtInvalid = 12
	} EventTypeEnum;
	static CStdString EventTypeToString(int eventTypeEnum);
	static int EventTypeToEnum(CStdString&);
	
	time_t m_timestamp;
	EventTypeEnum m_type;
	CStdString m_key;
	CStdString m_value;
};

typedef boost::shared_ptr<CaptureEvent> CaptureEventRef;


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



