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

#include "PacketHeaderDefs.h"
#include "SizedBuffer.h"
#include <list>
#include <map>
#include "LogManager.h"
#include <stdexcept>
#include "Utils.h"
#include "ace/Thread_Mutex.h"
CStdString logMsg;
static ACE_Thread_Mutex s_threadMutex;

// All the fragmentation handling algorithms must comply to this interface
class FragmentationAlgorithmInterface {
	public:
		virtual bool drop() = 0;
		virtual bool isComplete() = 0;
		virtual void addFragment(IpHeaderStruct *ipHeader) = 0;
		virtual time_t lastUpdate() = 0;
		virtual SizedBufferRef packetData() = 0;
		virtual ~FragmentationAlgorithmInterface() {} ;
};
typedef oreka::shared_ptr<FragmentationAlgorithmInterface> FragmentationAlgorithmRef; 

// Can only handle fragments coming inorder from first to last, without retransmission, can handle overlap
class ForwardSequentialAlgorithm : public FragmentationAlgorithmInterface {
	public:
		ForwardSequentialAlgorithm() : _drop(false), _isComplete(false), _lastUpdate(0), _packetDataRef() {};
		bool isComplete() {
			return _isComplete;
		}
		void addFragment(IpHeaderStruct *ipHeader) {
			try {
				if(!_packetDataRef) {
					if (ipHeader->offset()!=0) {
						_drop = true;
						return;
					}
					_packetDataRef.reset(new SizedBuffer((u_char*)ipHeader, ipHeader->packetLen()));
				}
				else {
					unsigned char* payload = reinterpret_cast<unsigned char*>(ipHeader)+ipHeader->headerLen();
					IpHeaderStruct* ipHeaderOriginal = reinterpret_cast<IpHeaderStruct*>(_packetDataRef->get());
					if (ipHeaderOriginal->payloadLen() != ipHeader->offset()) {
						_drop = true;
						return;
					}
					_packetDataRef->append(payload, ipHeader->payloadLen());
					ipHeaderOriginal = reinterpret_cast<IpHeaderStruct*>(_packetDataRef->get());
					ipHeaderOriginal->setPacketLen(ipHeaderOriginal->headerLen()+ipHeaderOriginal->payloadLen()+ipHeader->payloadLen());
				}

				if (ipHeader->isLastFragment()) {
					_isComplete = true;
				}
				_lastUpdate = time(NULL);
			}
			catch (const std::exception& ex) {
				FLOG_ERROR(LOG.ipfragmentation,"Caught exception will drop fragment id:%u, offset:%u, description: %s",ipHeader->packetId(), ipHeader->offset(), ex.what());
				_drop = true;
			}
		}
		SizedBufferRef packetData() {return _packetDataRef;};
		time_t lastUpdate() {return _lastUpdate;};
		bool drop() {return _drop;};
	private:
		bool _drop;
		SizedBufferRef _packetDataRef;
		time_t _lastUpdate;
		bool _isComplete;
} ;

// Can handle fragments coming in any order, can handle retransmission ,does not handle overlap
class WaitAllAndSortAlgorithm : public FragmentationAlgorithmInterface {
	public:
		WaitAllAndSortAlgorithm() : _isComplete(false), _drop(false), _currentSize(0), _finalSize(0), _lastUpdate(0) {};
		void addFragment(IpHeaderStruct *ipHeader) {
			try {
				// Find where the fragment belongs to and insert
				std::list<SizedBufferRef>::iterator it;
				for(it=_orderedFragments.begin();it!=_orderedFragments.end();it++) {
					IpHeaderStruct* header = (IpHeaderStruct*)(*it)->get();
					if (header->offset() == ipHeader->offset()) return; // retransmission
					if (header->offset() > ipHeader->offset()) break;
				}
				_orderedFragments.insert(it,SizedBufferRef(new SizedBuffer(reinterpret_cast<unsigned char*>(ipHeader),ipHeader->packetLen())));

				if (ipHeader->isLastFragment()) {
					_finalSize = ipHeader->offset() + ipHeader->payloadLen();
				}
				_currentSize += ipHeader->payloadLen();

				if (_finalSize != 0 && _currentSize > _finalSize) { // Possible overlap
					_drop = true;
					return;
				}

				if (_currentSize == _finalSize) {
					ForwardSequentialAlgorithm simpleHandler;
					for(std::list<SizedBufferRef>::iterator it=_orderedFragments.begin();it!=_orderedFragments.end();it++) {
						IpHeaderStruct* ipHeader = (IpHeaderStruct*)(*it)->get();
						simpleHandler.addFragment(ipHeader);
						if (simpleHandler.drop()) {
							_drop = true;
							return;
						}
					}
					_packetDataRef = simpleHandler.packetData();
					_isComplete = true;
				}
				_lastUpdate = time(NULL);
			}
			catch (const std::exception& ex) {
				FLOG_ERROR(LOG.ipfragmentation,"Caught exception will drop fragment id:%u, offset:%u, description: %s",ipHeader->packetId(), ipHeader->offset(), ex.what());
				_drop = true;
			}
		}

		SizedBufferRef packetData() {return _packetDataRef;};
		bool isComplete() { return _isComplete;}
		bool drop() {return _drop;};
		time_t lastUpdate() {return _lastUpdate;};
	private:
		std::list<SizedBufferRef> _orderedFragments;
		size_t _finalSize;
		time_t _lastUpdate;
		size_t _currentSize;
		bool _isComplete;
		bool _drop;
		SizedBufferRef _packetDataRef;
} ;

// Open/Closed Principle
// Set the desired fragmentation handling algortihm below
// No need to modify anywhere else in the code
#define FRAGMENTATION_ALGORITHM WaitAllAndSortAlgorithm

typedef std::map<unsigned short,FragmentationAlgorithmRef> FragmentMap; 

SizedBufferRef HandleIpFragment(IpHeaderStruct* ipHeader)
{
        MutexSentinel mutexSentinel(s_threadMutex);
	const time_t now = time(NULL);
	const int timeoutSec = 60;
	static time_t lastCheck = 0;
	static FragmentMap fragmentMap;

	// Delete previous fragments older than timeoutSec 
	if (now-lastCheck >= timeoutSec) {
		std::list <FragmentMap::iterator> toBeDeleted; 
		for (FragmentMap::iterator it=fragmentMap.begin(); it!=fragmentMap.end(); ++it) {
			if (now - it->second->lastUpdate() >= timeoutSec) 
				toBeDeleted.push_back(it);
		}
		if (toBeDeleted.size()) {
			for (std::list <FragmentMap::iterator>::iterator it=toBeDeleted.begin(); it!=toBeDeleted.end();++it) {
				fragmentMap.erase(*it);
			} 
			FLOG_WARN(LOG.ipfragmentation,"%d fragmented packets older than %u seconds has been removed from the map", toBeDeleted.size(), timeoutSec);
		}
		lastCheck = now;
	}

	FLOG_DEBUG(LOG.ipfragmentation,"Recieved fragment with packet id:%u, offset:%u, payload length:%u", ipHeader->packetId(), ipHeader->offset(), ipHeader->payloadLen());
	FragmentMap::iterator it = fragmentMap.find(ipHeader->packetId());
	if (it == fragmentMap.end()) { 
		it = fragmentMap.insert(std::make_pair(ipHeader->packetId(), FragmentationAlgorithmRef(new FRAGMENTATION_ALGORITHM()))).first;
	}
	FragmentationAlgorithmRef algorithm = it->second;
	algorithm->addFragment(ipHeader);

	if (algorithm->drop()) {
		fragmentMap.erase(it);
		FLOG_WARN(LOG.ipfragmentation,"Dropping fragmented packet data with id:%u", ipHeader->packetId());
	} 

	if (algorithm->isComplete()) {
		fragmentMap.erase(it);
		IpHeaderStruct *header = reinterpret_cast<IpHeaderStruct*>(algorithm->packetData()->get());
		FLOG_INFO(LOG.ipfragmentation, "Reassembled fragmented packet with id:%u, total payload length:%u", header->packetId(), header->payloadLen());
		return algorithm->packetData(); 
	}
	return SizedBufferRef(); // By default return empty ref
}
