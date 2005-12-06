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
#include "OrkBase.h"

#include "boost/shared_ptr.hpp"

/** This class represents a piece of audio. 
*/
class DLL_IMPORT_EXPORT AudioChunk
{
public: 
	typedef enum
	{
		UnknownAudio = 0,
		PcmAudio = 1,
		AlawAudio = 2,
		UlawAudio = 3,
		InvalidAudio = 4
	} AudioEncodingEnum;

	AudioChunk();
	~AudioChunk();

	/** Copy external buffer to internal buffer. Create internal buffer if necessary */
	void SetBuffer(void* pBuffer, size_t numBytes, AudioEncodingEnum, unsigned int timestamp = 0, unsigned int sequenceNumber = 0, unsigned int sampleRate = 8000);

	/** Computes the Root-Mean-Square power value of the buffer */
	double ComputeRms();
	/** Compute the RMS decibel value of the buffer with a 0 dB reference being the maximum theoretical RMS power of the buffer (2^15) */
	double ComputeRmsDb();

	int GetNumSamples();
	double GetDurationSec();

	AudioEncodingEnum m_encoding;
	unsigned int m_numBytes;
	void * m_pBuffer;
	unsigned int m_timestamp;			// usually: relative timestamp measured in samples
	unsigned int m_sequenceNumber;
	unsigned int m_sampleRate;
};

typedef boost::shared_ptr<AudioChunk> AudioChunkRef;

class DLL_IMPORT_EXPORT CaptureEvent
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
		EtInvalid = 8
	} EventTypeEnum;
	static CStdString EventTypeToString(int eventTypeEnum);
	static int EventTypeToEnum(CStdString&);
	
	time_t m_timestamp;
	EventTypeEnum m_type;
	CStdString m_key;
	CStdString m_value;
};

typedef boost::shared_ptr<CaptureEvent> CaptureEventRef;
#endif

