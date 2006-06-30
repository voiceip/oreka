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
#ifndef __TAPEPROCESSOR_H__
#define __TAPEPROCESSOR_H__

//#include <list>
//#include "ace/Singleton.h"
#include "AudioCapture.h"
#include "dll.h"
#include "OrkBase.h"

class TapeProcessor;

typedef boost::shared_ptr<TapeProcessor> TapeProcessorRef;

/** TapeProcessor Interface
 *  a filter is a black box that takes media chunks as an input and produces media chunks as an output
 *  it can be translating between two encodings (codec) or just processing the signal
 */
class DLL_IMPORT_EXPORT_ORKBASE TapeProcessor
{
public:
	virtual TapeProcessorRef __CDECL__ Instanciate() = 0;
	virtual void __CDECL__ AddAudioTape(AudioTapeRef audioTapeRef) = 0;
};

#endif