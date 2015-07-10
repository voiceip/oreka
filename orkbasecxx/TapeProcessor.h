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
#include "AudioTape.h"
#include "dll.h"
#include "OrkBase.h"


class DLL_IMPORT_EXPORT_ORKBASE TapeProcessor;

typedef oreka::shared_ptr<TapeProcessor> TapeProcessorRef;

/** TapeProcessor Interface
 *  a Tape Processor is a black box that takes Audio Tapes as an input and 
 *  processes them.
 */
class DLL_IMPORT_EXPORT_ORKBASE TapeProcessor
{
public:
	TapeProcessor();

	virtual CStdString __CDECL__ GetName() = 0;
	virtual TapeProcessorRef __CDECL__ Instanciate() = 0;
	virtual void __CDECL__ AddAudioTape(AudioTapeRef&) = 0;

	void SetNextProcessor(TapeProcessorRef& nextProcessor);
	void RunNextProcessor(AudioTapeRef&);

protected:
	TapeProcessorRef m_nextProcessor;
};

//===================================================================
/** TapeProcessor Registry
*/
class DLL_IMPORT_EXPORT_ORKBASE TapeProcessorRegistry
{
public:
	static TapeProcessorRegistry* instance();
	void RegisterTapeProcessor(TapeProcessorRef& TapeProcessor);
	TapeProcessorRef GetNewTapeProcessor(CStdString& TapeProcessorName);

	void RunProcessingChain(AudioTapeRef&);
	void CreateProcessingChain();
private:
	TapeProcessorRegistry();
	static TapeProcessorRegistry* m_singleton;

	std::list<TapeProcessorRef> m_TapeProcessors;
	TapeProcessorRef m_firstTapeProcessor;
};

#endif


