/*
 * Oreka -- A media capture and retrieval platform
 *
 * Copyright (C) 2005, orecx LLC
 *
 * http://www.orecx.com
 *
 */
#ifdef WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
//#include "winsock2.h"
#endif
#include "SipParsers.h"
#include "ParsingUtils.h"
#include "LogManager.h"
#include "VoIpConfig.h"
#include <boost/algorithm/string/predicate.hpp>
#include "MemUtils.h"

static LoggerPtr s_parsersLog = Logger::getLogger("parsers.sip");
static LoggerPtr s_sipPacketLog = Logger::getLogger("packet.sip");
static LoggerPtr s_sipExtractionLog = Logger::getLogger("sipextraction");
static LoggerPtr s_rtcpPacketLog = Logger::getLogger("packet.rtcp");
static LoggerPtr s_sipparsersLog = Logger::getLogger("packet.sipparsers");

extern VoIpConfigTopObjectRef g_VoIpConfigTopObjectRef;
#define DLLCONFIG g_VoIpConfigTopObjectRef.get()->m_config


// Return true if c is a char used to separate the key from the value in a sip key/value pair
inline bool IsAKeySeparator(char c)
{
	return (c == '=') || (c == ':') || (c == '<') || (c == '>');
}

// Return true if c is a char used to separate different keys or marks the end of a value in a sip key/value pair
inline bool IsAFieldSeparator(char c)
{
	return (c == ';') || (c == '<') || (c == '>') || (c == '\r') || (c == '\n');
}


//************************************
// Method:    OrkStringViewFind
// Returns:   the position of stringToFind inside searchedString
//			  npos  if not existent
// Parameter: const OrkStringView & searchedString   eg "ABCDEFG"
// Parameter: const OrkStringView & stringToFind     eg   "CDE"  will return result will be 2 
//************************************
size_t  OrkStringViewFind(const OrkStringView& searchedString, const OrkStringView& stringToFind)
{
	// replacement for string_view::find because 
	//  boost 1.53 on linux has a typo in the implementation (using a single = in place of ==)

	size_t locationInField = OrkStringView::npos;
#if (! defined(WIN32) && BOOST_VERSION<=105300)
	{
		auto it = std::search(searchedString.cbegin(), searchedString.cend(),
			stringToFind.cbegin(), stringToFind.cend());
		if (it != searchedString.cend())
		{
			locationInField = std::distance(searchedString.cbegin(), it);

		}
	}
#else
	locationInField = searchedString.find(stringToFind);
#endif
	return locationInField;
}



//************************************
// Method:    ExtractKeyValueFromField
// Returns:   OrkStringView  containing a range of bytes
//            This method searches for the substring between [*ptrFirstKeyChar  ... *ptrLastKeyChar]
//            in the payload that goes from [*pHeaderFirstChar .... *pHeaderFirstChar + maxLength] 
//            if the payload is: "INVITE sip:SIPREC-SRS@10.250.62.211:5060 SIP/2.0\r\nFrom:AAABBCCDD;ontact=12325654654\n..."
//            pHeaderFirstChar will point to "AAABBCCDD;ontact=12325654654\n...  "
//            and [*ptrFirstKeyChar  ... *ptrLastKeyChar]  will point to [contact]
//            when processing the from.contact for extraction

// Parameter: const char * pHeaderFirstChar  First byte where to search
// Parameter: size_t maxLength
// Parameter: const char * ptrFirstKeyChar  First char of the field
// Parameter: const char * ptrLastKeyChar   Last char of the field
//************************************
OrkStringView ExtractKeyValueFromField(const char* pHeaderFirstChar, size_t maxLength, const char* ptrFirstKeyChar, const char* ptrLastKeyChar)
{
	if (pHeaderFirstChar == nullptr || ptrFirstKeyChar == nullptr || ptrLastKeyChar == nullptr)
	{
		return OrkStringView();
	}

	// 1. Identify what's the exact area of the payload we are searching
	const char* pLastChar = pHeaderFirstChar;
	const char* pLastPayloadByte = (pHeaderFirstChar + maxLength);


	const char* pResultBegin = pHeaderFirstChar;
	const char* pResultEnd = pHeaderFirstChar;

	// Our last char will be either a carriage return or the last byte of the payload. 
	while (*pLastChar != '\r' && *pLastChar != '\n' && pLastChar < pLastPayloadByte) {
		++pLastChar;
	}

	OrkStringView stringToFind(ptrFirstKeyChar, ptrLastKeyChar - ptrFirstKeyChar);
	size_t locationInField = OrkStringViewFind(OrkStringView(pHeaderFirstChar, pLastChar - pHeaderFirstChar), stringToFind);

	if (locationInField != OrkStringView::npos)
	{
		// the filed has been located,  skip irrelevant prefixes and get the result until a separator is encountred
		// Pass all separators;
		pResultBegin = pHeaderFirstChar + locationInField + stringToFind.length();

		while ((IsAKeySeparator(*pResultBegin) || ((*pResultBegin) == ' ')) && pResultBegin < pLastChar) {
			++pResultBegin;
		}
		pResultEnd = pResultBegin;
		// Locate last meaningful char 
		while (!IsAFieldSeparator(*pResultEnd) && (*pResultEnd != '\n') && (*pResultEnd != '\r') && (*pResultEnd != 0) && (pResultEnd < pLastChar)) {
			++pResultEnd;
		}

	}
	return OrkStringView(pResultBegin, pResultEnd - pResultBegin);
}


//************************************
// Method:    retrieve_field_position_in_payload
// Returns:   bool  
//            if ptrFirstChar is not null, it's returned as is.  Otherwise, it's updated (for future reuse) with the location of the fieldName in the payload
// Parameter: const char *   payload         Buffer to search
// Parameter:     size_t     payLoadLength   Size of the buffer
// Parameter: const char * & ptrFirstChar    Previously computed value to update if null
// Parameter: const char *   fieldName       String we search for
//************************************
bool retrieve_field_position_in_payload(const char* payload, size_t payLoadLength, const char*& ptrFirstChar, const  char* fieldName)
{
	if (!ptrFirstChar)
	{
		ptrFirstChar = MemFindAfter(const_cast<char*> (fieldName), const_cast<char*> (payload), const_cast<char*> (payload) + payLoadLength);
		if (ptrFirstChar) {
			ptrFirstChar += strlen(fieldName);
		}
	}
	return (ptrFirstChar != nullptr);

}




//************************************
// Method:    ExtractKeyValuesData
// 
// Parameter: std::map<CStdString , CStdString> & keyValueMapToFill  : A reference to a map to fill 
// Parameter: std::array<const char *eMaxSearchFieldPosition > fieldPosition : An array of know locations in the payload for some standard sip records (from, to ...)
// Parameter: char * payload   : first byte of the payload
// Parameter: size_t payloadSize : length of the payload
//************************************
void ExtractKeyValuesData(std::map<CStdString, CStdString>& keyValueMapToFill, std::array<const char*, eMaxSearchFieldPosition > fieldPosition, char* payload, size_t payloadSize)
{
	// This function will extract  <Key,Value> pairs and append them to the keyValueMapToFill map.
	// For each key in the m_sipExtractKeyValues list
	//	  a. It first search in the precomputed array fieldPosition if the key matches it, the proceed to the extraction
	//    b. If not found, we test if the key has a dot in its middle
	//       b1. if yes,  find the sip line that starts with the left part of the key 
	//	  c. if the key does not have a dot then a search across all the payload is done to locate the first string that matches the key
	

	//	  part a: It first search in the precomputed array fieldPosition if the key matches it, the proceed to the extraction

	for (const CStdString& x : DLLCONFIG.m_sipExtractKeyValues)
	{
		OrkStringView keyData;

		// First check if the value to extract matches one of the standard precomputed field . 

		if (x.substr(0, 5) == "from.") {
			if (!retrieve_field_position_in_payload(payload, payloadSize, fieldPosition[eSearch_FieldFrom_Position], "from:"))
			{
				if (s_sipPacketLog->isDebugEnabled())
				{
					CStdString logMsg;
					logMsg.Format("Extracting SIP Key/Value %s: failed to find the standard field 'from' in the payload", x.c_str());
					LOG4CXX_DEBUG(s_sipPacketLog, logMsg);
				}
				continue;
			}
			keyData = ExtractKeyValueFromField(fieldPosition[eSearch_FieldFrom_Position], fieldPosition[eSearch_FieldFrom_Position] - payload + payloadSize, &x[5], x.c_str() + x.length());
		}
		else if (x.substr(0, 3) == "to.") {
			if (!retrieve_field_position_in_payload(payload, payloadSize, fieldPosition[eSearch_FieldTo_Position], "to:"))
			{
				if (s_sipPacketLog->isDebugEnabled())
				{
					CStdString logMsg;
					logMsg.Format("Extracting SIP Key/Value %s: failed to find the standard field 'to' in the payload", x.c_str());
					LOG4CXX_DEBUG(s_sipPacketLog, logMsg);
				}
				continue;
			}
			keyData = ExtractKeyValueFromField(fieldPosition[eSearch_FieldTo_Position], fieldPosition[eSearch_FieldTo_Position] - payload + payloadSize, &x[3], x.c_str() + x.length());
		}
		else if (x.substr(0, 9) == "replaces.") {
			if (!retrieve_field_position_in_payload(payload, payloadSize, fieldPosition[eSearch_FieldReplaces_Position], "replaces:"))
			{
				if (s_sipPacketLog->isDebugEnabled())
				{
					CStdString logMsg;
					logMsg.Format("Extracting SIP Key/Value %s: failed to find the standard field 'replaces' in the payload", x.c_str());
					LOG4CXX_DEBUG(s_sipPacketLog, logMsg);
				}
				continue;
			}
			keyData = ExtractKeyValueFromField(fieldPosition[eSearch_FieldReplaces_Position], fieldPosition[eSearch_FieldReplaces_Position] - payload + payloadSize, &x[9], x.c_str() + x.length());
		}
		else if (x.substr(0, 8) == "contact.") {
			if (!retrieve_field_position_in_payload(payload, payloadSize, fieldPosition[eSearch_FieldContact_Position], "contact:"))
			{
				if (s_sipPacketLog->isDebugEnabled())
				{
					CStdString logMsg;
					logMsg.Format("Extracting SIP Key/Value %s: failed to find the 'contact' standard field in the payload", x.c_str());
					LOG4CXX_DEBUG(s_sipPacketLog, logMsg);
				}
				continue;
			}
			keyData = ExtractKeyValueFromField(fieldPosition[eSearch_FieldContact_Position], fieldPosition[eSearch_FieldContact_Position] - payload + payloadSize, &x[8], x.c_str() + x.length());
		}
		else {

			// Perform a search in the payload

			size_t currentStartSearchPos = 0;
			size_t dotPosition = x.find('.', currentStartSearchPos);
			size_t locationInPayload = OrkStringView::npos;

			//    b. If not found, we test if the key has a dot in its middle
			if (dotPosition != OrkStringView::npos)
			{
				// Looking for aaaaa.bbbbb   
				// Find the line that starts with '\naaaaa:'
				// then extract from this line the value for field bbbbb 

				bool foundTag = false;
				bool reachedEndOfBuffer = false;

				OrkStringView bufferToSearch(payload, payloadSize);
				OrkStringView keyToLookUp(x.c_str(), dotPosition);

				while (!foundTag && !reachedEndOfBuffer && bufferToSearch.size() > 0)
				{
					size_t posSubStr = OrkStringViewFind(bufferToSearch, keyToLookUp);
					if (posSubStr == OrkStringView::npos)
					{
						reachedEndOfBuffer = true;
					}
					else
					{
						bool foundExtractionArea = false;
						bool failedToFindAtStartOfLine = false;

						// Make sure that the we have '\n{spaces - tabs}*sip_key:sip values", 
						// still take care that the first sip line does not start with a '\n'
						size_t tmpPosSubStr = posSubStr;
						while ((bufferToSearch[tmpPosSubStr] == ' ' || bufferToSearch[tmpPosSubStr] == '\t') && (tmpPosSubStr > 0)) --tmpPosSubStr;
						if (tmpPosSubStr != 0 && (bufferToSearch[tmpPosSubStr - 1] != '\n')) {
							failedToFindAtStartOfLine = true;
						}
						// Do we have a ':' after the sip field name?
						if (! failedToFindAtStartOfLine) {
							foundExtractionArea = (bufferToSearch[posSubStr + keyToLookUp.size()] == ':');
						}

						//       b1:  found the sip line that starts with the left part of the key 
						if (foundExtractionArea)
						{
							// Try to find the key from this line.
							// in the search for aaaa.bbbbb strToFind will be set to bbbbb
							OrkStringView strToFind(x.c_str() + dotPosition + 1, x.length() - dotPosition - 1);

							OrkStringView subpartToSearchIn = bufferToSearch.substr(posSubStr + keyToLookUp.size() + 1);

							locationInPayload = OrkStringViewFind(subpartToSearchIn, strToFind);
							if (locationInPayload != OrkStringView::npos)
							{
								locationInPayload += (&subpartToSearchIn[0] - payload) + strToFind.length();
							}
							foundTag = true;
						}
						else
						{
							// Skip to next line. 
							size_t nextLinePos = bufferToSearch.find('\n');
							if (nextLinePos != OrkStringView::npos)
							{
								bufferToSearch = &bufferToSearch[nextLinePos + 1];
							}
							else
							{
								reachedEndOfBuffer = true;
							}
						}
					}
				}
				if (locationInPayload == OrkStringView::npos)
				{
					// Failed to find a sip method that starts with the left part of x
					if (s_sipPacketLog->isDebugEnabled())
					{
						CStdString logMsg;
						logMsg.Format("Extracting SIP Key/Value %s:failure. Check that the left part of '%s' well spelled", x.c_str());
						LOG4CXX_DEBUG(s_sipPacketLog, logMsg);
					}
				}
			}
			else
			{
				// c. search across all the payload to locate the first string that matches the key
                locationInPayload =  OrkStringViewFind(OrkStringView(payload, payloadSize), OrkStringView(x.c_str(), x.length()));
				if (locationInPayload != OrkStringView::npos)
				{
					locationInPayload += x.length();
				}
			}


			if (locationInPayload != OrkStringView::npos)
			{
				// Pass all separators;
				const char* pResultBegin = payload + locationInPayload;
				const char* pLastChar = payload + payloadSize;

				while ((IsAKeySeparator(*pResultBegin) || ((*pResultBegin) == ' ')) && pResultBegin < pLastChar) {
					++pResultBegin;
				}

				const char* pResultEnd = pResultBegin;
				while (!IsAFieldSeparator(*pResultEnd) && (*pResultEnd != '\n') && (*pResultEnd != '\r') && (*pResultEnd != 0) && (pResultEnd < pLastChar)) {
					++pResultEnd;
				}
				keyData = OrkStringView(pResultBegin, pResultEnd - pResultBegin);
			}

		}

		if ( ! keyData.empty())
		{
			keyValueMapToFill.emplace(x, CStdString(keyData.begin(), keyData.length()));
			if (s_sipPacketLog->isDebugEnabled())
			{
				CStdString logMsg;
				logMsg.Format("Extracting SIP Key/Value %s:success  value:%s", x.c_str(), keyValueMapToFill[x].c_str());
				LOG4CXX_DEBUG(s_sipPacketLog, logMsg);
			}

		}
		else
		{
			if (s_sipPacketLog->isDebugEnabled())
			{
				CStdString logMsg;
				logMsg.Format("Extracting SIP Key/Value %s:failure", x.c_str());
				LOG4CXX_DEBUG(s_sipPacketLog, logMsg);
			}
		}
	}
}



bool TrySipBye(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, UdpHeaderStruct* udpHeader, u_char* udpPayload, u_char* packetEnd)
{
	bool result = false;

	int sipLength = ntohs(udpHeader->len) - sizeof(UdpHeaderStruct);
	char* sipEnd = (char*)udpPayload + sipLength;
	if(sipLength < (int)sizeof(SIP_METHOD_BYE) || sipEnd > (char*)packetEnd)
	{
		return false;
	}

	if (memcmp(SIP_METHOD_BYE, (void*)udpPayload, SIP_METHOD_BYE_SIZE) == 0)
	{
		result = true;
		SipByeInfoRef info(new SipByeInfo());

		char* fromField = memFindAfter("From:", (char*)udpPayload, sipEnd);
		if(!fromField)
		{
			fromField = memFindAfter("\nf:", (char*)udpPayload, sipEnd);
		}
		char* toField = memFindAfter("To:", (char*)udpPayload, sipEnd);
		if(!toField)
		{
			toField = memFindAfter("\nt:", (char*)udpPayload, sipEnd);
		}

		char* callIdField = memFindAfter("Call-ID:", (char*)udpPayload, sipEnd);
		if(!callIdField)
		{
			callIdField = memFindAfter("\ni:", (char*)udpPayload, sipEnd);
		}
		if(callIdField)
		{
			GrabTokenSkipLeadingWhitespaces(callIdField, sipEnd, info->m_callId);
		}

		if(fromField)
		{
			if(s_sipExtractionLog->isDebugEnabled())
			{
				CStdString from;
				GrabLine(fromField, sipEnd, from);
				LOG4CXX_DEBUG(s_sipExtractionLog, "from: " + from);
			}

			char* fromFieldEnd = memFindEOL(fromField, sipEnd);

			GrabSipName(fromField, fromFieldEnd, info->m_fromName);

			char* sipUser = memFindAfter("sip:", fromField, fromFieldEnd);
			if(sipUser)
			{
				if(DLLCONFIG.m_sipReportFullAddress)
				{
					GrabSipUserAddress(sipUser, fromFieldEnd, info->m_from);
				}
				else
				{
					GrabSipUriUser(sipUser, fromFieldEnd, info->m_from);
				}
				GrabSipUriDomain(sipUser, fromFieldEnd, info->m_fromDomain);
			}
			else
			{
				if(DLLCONFIG.m_sipReportFullAddress)
				{
					GrabSipUserAddress(fromField, fromFieldEnd, info->m_from);
				}
				else
				{
					GrabSipUriUser(fromField, fromFieldEnd, info->m_from);
				}
				GrabSipUriDomain(fromField, fromFieldEnd, info->m_fromDomain);
			}
		}
		if(toField)
		{
			CStdString to;
			char* toFieldEnd = GrabLine(toField, sipEnd, to);
			LOG4CXX_DEBUG(s_sipExtractionLog, "to: " + to);

			GrabSipName(toField, toFieldEnd, info->m_toName);

			char* sipUser = memFindAfter("sip:", toField, toFieldEnd);
			if(sipUser)
			{
				if(DLLCONFIG.m_sipReportFullAddress)
				{
					GrabSipUserAddress(sipUser, toFieldEnd, info->m_to);
				}
				else
				{
					GrabSipUriUser(sipUser, toFieldEnd, info->m_to);
				}
				GrabSipUriDomain(sipUser, toFieldEnd, info->m_toDomain);
			}
			else
			{
				if(DLLCONFIG.m_sipReportFullAddress)
				{
					GrabSipUserAddress(toField, toFieldEnd, info->m_to);
				}
				else
				{
					GrabSipUriUser(toField, toFieldEnd, info->m_to);
				}
				GrabSipUriDomain(toField, toFieldEnd, info->m_toDomain);
			}
		}

		info->m_senderIp = ipHeader->ip_src;
		info->m_receiverIp = ipHeader->ip_dest;

		CStdString logMsg;
		info->ToString(logMsg);
		LOG4CXX_INFO(s_sipPacketLog, "BYE: " + logMsg);
		if(callIdField && DLLCONFIG.m_sipIgnoreBye == false)
		{
			VoIpSessionsSingleton::instance()->ReportSipBye(info);
		}
	}
	return result;
}

bool TrySipNotify(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, UdpHeaderStruct* udpHeader, u_char* udpPayload, u_char* packetEnd)
{
	int sipLength = ntohs(udpHeader->len) - sizeof(UdpHeaderStruct);
	char* sipEnd = (char*)udpPayload + sipLength;
	if(sipLength < (int)sizeof(SIP_METHOD_BYE) || sipEnd > (char*)packetEnd)
	{
		return false;
	}

	if (boost::starts_with((char*)udpPayload, "NOTIFY")) 
	{
		SipNotifyInfoRef info(new SipNotifyInfo());
		info->m_senderIp = ipHeader->ip_src;
		info->m_receiverIp = ipHeader->ip_dest;

		char* callIdField = memFindAfter("Call-ID:", (char*)udpPayload, sipEnd);
		if(!callIdField)
		{
			callIdField = memFindAfter("\ni:", (char*)udpPayload, sipEnd);
		}
		if(callIdField)
		{
			GrabTokenSkipLeadingWhitespaces(callIdField, sipEnd, info->m_callId);
		}

		char* dspField = memFindAfter(DLLCONFIG.m_necNotifyDispLine, (char*)udpPayload, sipEnd);
		if(!dspField)
		{
			dspField = memFindAfter("\nt:", (char*)udpPayload, sipEnd);
		}
		if(dspField)
		{
			char* contentLen = memFindAfter("Content-Length: 50", (char*)udpPayload, sipEnd);
			if (contentLen) {
				dspField = contentLen+50;
				while (*dspField != ' ') dspField--; // find the position of the lastest token before end
			}

			GrabTokenSkipLeadingWhitespaces(dspField, sipEnd, info->m_dsp);

			CStdString hex;
			for(int i=0;i<info->m_dsp.length();i++) {
				char byteAsHex[32];
				sprintf(byteAsHex, "%.2x", *(info->m_dsp.c_str()+i));
				hex += byteAsHex;
			}

			if (hex == DLLCONFIG.m_necOnHoldMarker) {
				info->m_onHoldMarker = true;
			}

			if (hex == DLLCONFIG.m_necOffHoldMarker) {
				info->m_offHoldMarker = true;
			}
		}

		char* pkeyField = memFindAfter("Ind-Pkey=", (char*)udpPayload, sipEnd);

		if (!pkeyField) {
			pkeyField = memFindAfter("Ind-Fkey=", (char*)udpPayload, sipEnd);
		}

		if (pkeyField) {
			CStdString pkeyToken;
			GrabTokenSkipLeadingWhitespaces(pkeyField, sipEnd, pkeyToken);
			sscanf(pkeyToken.c_str(),"%d",&info->m_pkey);
		}

		if (callIdField && DLLCONFIG.m_sipNotifySupport)
		{
			LOG4CXX_INFO(s_sipPacketLog, "NOTIFY: " + info->ToString());
			VoIpSessionsSingleton::instance()->ReportSipNotify(info);
			return true;
		}
	}
	return false;
}

bool TrySipInfo(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, UdpHeaderStruct* udpHeader, u_char* udpPayload, u_char* packetEnd)
{
	bool result = false;

	int sipLength = ntohs(udpHeader->len) - sizeof(UdpHeaderStruct);
	char* sipEnd = (char*)udpPayload + sipLength;
	if(sipLength < (int)sizeof(SIP_METHOD_INFO) || sipEnd > (char*)packetEnd)
	{
		return false;
	}

	if (memcmp(SIP_METHOD_INFO, (void*)udpPayload, SIP_INFO_SIZE) == 0)
	{
		result = true;

		SipInfoRef info(new SipInfo());
		char* callIdField = memFindAfter("Call-ID:", (char*)udpPayload, sipEnd);
		if(!callIdField)
		{
			callIdField = memFindAfter("\ni:", (char*)udpPayload, sipEnd);
		}
		if(callIdField)
		{
			GrabTokenSkipLeadingWhitespaces(callIdField, sipEnd, info->m_callId);
		}
		if(info->m_callId.length() < 1)
		{
			return true;
		}

		char* cSeqField = memFindAfter("CSeq:", (char*)udpPayload, sipEnd);
		if(cSeqField)
		{
			GrabTokenSkipLeadingWhitespaces(cSeqField, sipEnd, info->m_cSeq);
		}

		char* signalField = memFindAfter("Signal=", (char*)udpPayload, (char*)sipEnd);
		if(signalField)
		{
			CStdString dtmfDigitStr;
			GrabTokenSkipLeadingWhitespaces(signalField, sipEnd, info->m_dtmfDigit);
		}

		CStdString ondemandFieldName;
		ondemandFieldName.Format("%s:", DLLCONFIG.m_sipOnDemandFieldName);
		char* recordField = memFindAfter(ondemandFieldName, (char*)udpPayload, (char*)sipEnd);
		if(recordField)
		{
			CStdString field;
			GrabLineSkipLeadingWhitespace(recordField, sipEnd, field);
			if(field.CompareNoCase(DLLCONFIG.m_sipOnDemandFieldValue) == 0)
			{
				info->m_onDemand = true;
				info->m_onDemandOff = false;
			}
			else if(field.CompareNoCase(DLLCONFIG.m_sipOnDemandFieldValueOff) == 0)
			{
				info->m_onDemand = false;
				info->m_onDemandOff = true;
			}

		}
		VoIpSessionsSingleton::instance()->ReportSipInfo(info);
	}
	return result;

}

bool TryLogFailedSip(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, UdpHeaderStruct* udpHeader, u_char* udpPayload, u_char* packetEnd)
{
	if(DLLCONFIG.m_sipLogFailedCalls == false)
	{
		return false;
	}

	int sipLength = ntohs(udpHeader->len) - sizeof(UdpHeaderStruct);
	char* sipEnd = (char*)udpPayload + sipLength;
	CStdString callId, errorCode, logMsg, errorString;

	if(sipLength < 9 || sipEnd > (char*)packetEnd)
	{
		return false;
	}

	if((memcmp("SIP/2.0 4", (void*)udpPayload, 9) == 0) ||
	   (memcmp("SIP/2.0 5", (void*)udpPayload, 9) == 0) ||
	   (memcmp("SIP/2.0 6", (void*)udpPayload, 9) == 0) ||
	   (memcmp("CANCEL ", (void*)udpPayload, 7) == 0))
	{
		char* callIdField = memFindAfter("Call-ID:", (char*)udpPayload, sipEnd);
		if(!callIdField)
		{
			callIdField = memFindAfter("\ni:", (char*)udpPayload, sipEnd);
		}
		char* eCode = memFindAfter("SIP/2.0 ", (char*)udpPayload, sipEnd);

		if(callIdField)
		{
			GrabTokenSkipLeadingWhitespaces(callIdField, sipEnd, callId);
		}

		if((memcmp("CANCEL ", (void*)udpPayload, 7) == 0))
		{
			errorCode.Format("CANCEL");
			errorString.Format("User Agent CANCEL");
		}
		else
		{
			if(eCode)
			{
				GrabTokenSkipLeadingWhitespaces(eCode, sipEnd, errorCode);
				GrabLine((eCode+errorCode.size()+1), sipEnd, errorString);
			}
		}
	}

	if(!(callId.size() && errorCode.size()))
	{
		return false;
	}

	SipFailureMessageInfoRef info(new SipFailureMessageInfo());
	info->m_senderIp = ipHeader->ip_src;
	info->m_receiverIp = ipHeader->ip_dest;
	memcpy(info->m_senderMac, ethernetHeader->sourceMac, sizeof(info->m_senderMac));
	memcpy(info->m_receiverMac, ethernetHeader->destinationMac, sizeof(info->m_receiverMac));
	info->m_callId = callId;
	info->m_errorCode = errorCode;
	info->m_errorString = errorString;

	// Logging is done in VoIpSessions.cpp
	//CStdString sipError;

	//info->ToString(sipError);
	//LOG4CXX_INFO(s_sipPacketLog, "SIP Error packet: " + sipError);

	VoIpSessionsSingleton::instance()->ReportSipErrorPacket(info);

	return true;
}

// Not used in the case of SIP over TCP (183 Session Progress parsed by TrySipInvite) - do the same for SIP over TCP at some point?
bool TrySipSessionProgress(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, UdpHeaderStruct* udpHeader, u_char* udpPayload, u_char* packetEnd)
{
	bool result = false;

	int sipLength = ntohs(udpHeader->len) - sizeof(UdpHeaderStruct);
	char* sipEnd = (char*)udpPayload + sipLength;

	if(sipLength < SIP_RESPONSE_SESSION_PROGRESS_SIZE || sipEnd > (char*)packetEnd)
	{
		;	// packet too short
	}
	else if(memcmp(SIP_RESPONSE_SESSION_PROGRESS, (void*)udpPayload, SIP_RESPONSE_SESSION_PROGRESS_SIZE) == 0)
	{
		bool hasSdp = false;
		SipSessionProgressInfoRef info(new SipSessionProgressInfo());

		result = true;

		char* fromField = memFindAfter("From:", (char*)udpPayload, sipEnd);
		if(!fromField)
		{
			fromField = memFindAfter("\nf:", (char*)udpPayload, sipEnd);
		}
		char* toField = memFindAfter("To:", (char*)udpPayload, sipEnd);
		if(!toField)
		{
			toField = memFindAfter("\nt:", (char*)udpPayload, sipEnd);
		}

		char* callIdField = memFindAfter("Call-ID:", (char*)udpPayload, sipEnd);
		if(!callIdField)
		{
			callIdField = memFindAfter("\ni:", (char*)udpPayload, sipEnd);
		}

		char* audioField = NULL;
		char* connectionAddressField = NULL;

		if(callIdField)
		{
			GrabTokenSkipLeadingWhitespaces(callIdField, sipEnd, info->m_callId);
			audioField = memFindAfter("m=audio ", callIdField, sipEnd);
			connectionAddressField = memFindAfter("c=IN IP4 ", callIdField, sipEnd);
		}
		if(audioField && connectionAddressField)
		{
			hasSdp = true;

			GrabToken(audioField, sipEnd, info->m_mediaPort);

			CStdString connectionAddress;
			GrabToken(connectionAddressField, sipEnd, connectionAddress);
			struct in_addr mediaIp;
			if(connectionAddress.size())
			{
				if(inet_pton4((PCSTR)connectionAddress, &mediaIp))
				{
					info->m_mediaIp = mediaIp;
				}
			}
		}

		if(fromField)
		{
			if(s_sipExtractionLog->isDebugEnabled())
			{
				CStdString from;
				GrabLine(fromField, sipEnd, from);
				LOG4CXX_DEBUG(s_sipExtractionLog, "from: " + from);
			}

			char* fromFieldEnd = memFindEOL(fromField, sipEnd);

			char* sipUser = memFindAfter("sip:", fromField, fromFieldEnd);
			if(sipUser)
			{
				if(DLLCONFIG.m_sipReportFullAddress)
				{
					GrabSipUserAddress(sipUser, fromFieldEnd, info->m_from);
				}
				else
				{
					GrabSipUriUser(sipUser, fromFieldEnd, info->m_from);
				}
			}
			else
			{
				if(DLLCONFIG.m_sipReportFullAddress)
				{
					GrabSipUserAddress(fromField, fromFieldEnd, info->m_from);
				}
				else
				{
					GrabSipUriUser(fromField, fromFieldEnd, info->m_from);
				}
			}
		}
		if(toField)
		{
			CStdString to;
			char* toFieldEnd = GrabLine(toField, sipEnd, to);
			LOG4CXX_DEBUG(s_sipExtractionLog, "to: " + to);

			char* sipUser = memFindAfter("sip:", toField, toFieldEnd);
			if(sipUser)
			{
				if(DLLCONFIG.m_sipReportFullAddress)
				{
					GrabSipUserAddress(sipUser, toFieldEnd, info->m_to);
				}
				else
				{
					GrabSipUriUser(sipUser, toFieldEnd, info->m_to);
				}
			}
			else
			{
				if(DLLCONFIG.m_sipReportFullAddress)
				{
					GrabSipUserAddress(toField, toFieldEnd, info->m_to);
				}
				else
				{
					GrabSipUriUser(toField, toFieldEnd, info->m_to);
				}
			}
		}
		info->m_senderIp = ipHeader->ip_src;
		info->m_originalSenderIp = ipHeader->ip_src;
		info->m_receiverIp = ipHeader->ip_dest;

		CStdString logMsg;

		info->ToString(logMsg);
		logMsg = "183 Session Progress: " + logMsg;
		if(!hasSdp)
		{
			logMsg = logMsg + " dropped because it lacks the SDP";
		}
		else
		{
			VoIpSessionsSingleton::instance()->ReportSipSessionProgress(info);
		}
		LOG4CXX_INFO(s_sipPacketLog, logMsg);
	}
	return result;
}

bool TrySip200Ok(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, UdpHeaderStruct* udpHeader, u_char* udpPayload, u_char* packetEnd)
{
	bool result = false;

	if(DLLCONFIG.m_sipTreat200OkAsInvite == true)
	{
		return false;
	}

	int sipLength = ntohs(udpHeader->len) - sizeof(UdpHeaderStruct);
	char* sipEnd = (char*)udpPayload + sipLength;
	if(sipLength < SIP_RESPONSE_200_OK_SIZE || sipEnd > (char*)packetEnd)
	{
		;	// packet too short
	}
	else if (memcmp(SIP_RESPONSE_200_OK, (void*)udpPayload, SIP_RESPONSE_200_OK_SIZE) == 0)
	{
		result = true;

		Sip200OkInfoRef info(new Sip200OkInfo());

		char* fromField = memFindAfter("From:", (char*)udpPayload, sipEnd);
		if(!fromField)
		{
			fromField = memFindAfter("\nf:", (char*)udpPayload, sipEnd);
		}
		char* toField = memFindAfter("To:", (char*)udpPayload, sipEnd);
		if(!toField)
		{
			toField = memFindAfter("\nt:", (char*)udpPayload, sipEnd);
		}

		char* callIdField = memFindAfter("Call-ID:", (char*)udpPayload, sipEnd);
		if(!callIdField)
		{
			callIdField = memFindAfter("\ni:", (char*)udpPayload, sipEnd);
		}

		char* audioField = NULL;
		char* connectionAddressField = NULL;

		if(callIdField)
		{
			GrabTokenSkipLeadingWhitespaces(callIdField, sipEnd, info->m_callId);
			audioField = memFindAfter("m=audio ", callIdField, sipEnd);
			connectionAddressField = memFindAfter("c=IN IP4 ", callIdField, sipEnd);
		}
		if(audioField && connectionAddressField)
		{
			info->m_hasSdp = true;

			GrabToken(audioField, sipEnd, info->m_mediaPort);

			CStdString connectionAddress;
			GrabToken(connectionAddressField, sipEnd, connectionAddress);
			struct in_addr mediaIp;
			if(connectionAddress.size())
			{
				if(inet_pton4((PCSTR)connectionAddress, &mediaIp))
				{
					info->m_mediaIp = mediaIp;
				}
			}
		}

		if(fromField)
		{
			if(s_sipExtractionLog->isDebugEnabled())
			{
				CStdString from;
				GrabLine(fromField, sipEnd, from);
				LOG4CXX_DEBUG(s_sipExtractionLog, "from: " + from);
			}

			char* fromFieldEnd = memFindEOL(fromField, sipEnd);

			char* sipUser = memFindAfter("sip:", fromField, fromFieldEnd);
			if(sipUser)
			{
				if(DLLCONFIG.m_sipReportFullAddress)
				{
					GrabSipUserAddress(sipUser, fromFieldEnd, info->m_from);
				}
				else
				{
					GrabSipUriUser(sipUser, fromFieldEnd, info->m_from);
				}
			}
			else
			{
				if(DLLCONFIG.m_sipReportFullAddress)
				{
					GrabSipUserAddress(fromField, fromFieldEnd, info->m_from);
				}
				else
				{
					GrabSipUriUser(fromField, fromFieldEnd, info->m_from);
				}
			}
		}
		if(toField)
		{
			CStdString to;
			char* toFieldEnd = GrabLine(toField, sipEnd, to);
			LOG4CXX_DEBUG(s_sipExtractionLog, "to: " + to);

			char* sipUser = memFindAfter("sip:", toField, toFieldEnd);
			if(sipUser)
			{
				if(DLLCONFIG.m_sipReportFullAddress)
				{
					GrabSipUserAddress(sipUser, toFieldEnd, info->m_to);
				}
				else
				{
					GrabSipUriUser(sipUser, toFieldEnd, info->m_to);
				}
			}
			else
			{
				if(DLLCONFIG.m_sipReportFullAddress)
				{
					GrabSipUserAddress(toField, toFieldEnd, info->m_to);
				}
				else
				{
					GrabSipUriUser(toField, toFieldEnd, info->m_to);
				}
			}
		}
		info->m_senderIp = ipHeader->ip_src;
		info->m_receiverIp = ipHeader->ip_dest;

		CStdString logMsg;

		info->ToString(logMsg);
		logMsg = "200 OK: " + logMsg;
		if(info->m_hasSdp)
		{
			LOG4CXX_INFO(s_sipPacketLog, logMsg);
		}
		else
		{
			LOG4CXX_DEBUG(s_sipPacketLog, logMsg);
		}

		VoIpSessionsSingleton::instance()->ReportSip200Ok(info);
	}
	return result;
}

bool TrySip302MovedTemporarily(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, UdpHeaderStruct* udpHeader, u_char* udpPayload, u_char* packetEnd)
{
	bool result = false;
	bool drop = false;

	if(DLLCONFIG.m_sip302MovedTemporarilySupport == false)
	{
		return false;
	}

	int sipLength = ntohs(udpHeader->len) - sizeof(UdpHeaderStruct);
	char* sipEnd = (char*)udpPayload + sipLength;
	CStdString sipMethod = "302 Moved Temporarily";

	if(sipLength < SIP_RESPONSE_302_MOVED_TEMPORARILY_SIZE || sipEnd > (char*)packetEnd)
	{
		drop = true;	// packet too short
	}
	else if(memcmp(SIP_RESPONSE_302_MOVED_TEMPORARILY, (void*)udpPayload, SIP_RESPONSE_302_MOVED_TEMPORARILY_SIZE) != 0)
	{
		drop = true;
	}

	if(drop == false)
	{
		result = true;

		Sip302MovedTemporarilyInfoRef info(new Sip302MovedTemporarilyInfo());

		char* fromField = memFindAfter("From:", (char*)udpPayload, sipEnd);
		if(!fromField)
		{
			fromField = memFindAfter("\nf:", (char*)udpPayload, sipEnd);
		}
		char* toField = memFindAfter("To:", (char*)udpPayload, sipEnd);
		if(!toField)
		{
			toField = memFindAfter("\nt:", (char*)udpPayload, sipEnd);
		}
		char* callIdField = memFindAfter("Call-ID:", (char*)udpPayload, sipEnd);
		if(!callIdField)
		{
			callIdField = memFindAfter("\ni:", (char*)udpPayload, sipEnd);
		}
		char* contactField = memFindAfter("Contact:", (char*)udpPayload, sipEnd);
		if(!contactField)
		{
			contactField = memFindAfter("\nc:", (char*)udpPayload, sipEnd);
		}

		if(fromField)
		{
			if(s_sipExtractionLog->isDebugEnabled())
			{
				CStdString from;
				GrabLine(fromField, sipEnd, from);
				LOG4CXX_DEBUG(s_sipExtractionLog, "from: " + from);
			}

			char* fromFieldEnd = memFindEOL(fromField, sipEnd);

			GrabSipName(fromField, fromFieldEnd, info->m_fromName);

			char* sipUser = memFindAfter("sip:", fromField, fromFieldEnd);
			if(sipUser)
			{
				if(DLLCONFIG.m_sipReportFullAddress)
				{
					GrabSipUserAddress(sipUser, fromFieldEnd, info->m_from);
				}
				else
				{
					GrabSipUriUser(sipUser, fromFieldEnd, info->m_from);
				}
				GrabSipUriDomain(sipUser, fromFieldEnd, info->m_fromDomain);
			}
			else
			{
				if(DLLCONFIG.m_sipReportFullAddress)
				{
					GrabSipUserAddress(fromField, fromFieldEnd, info->m_from);
				}
				else
				{
					GrabSipUriUser(fromField, fromFieldEnd, info->m_from);
				}
				GrabSipUriDomain(fromField, fromFieldEnd, info->m_fromDomain);
			}
		}
		if(toField)
		{
			CStdString to;
			char* toFieldEnd = GrabLine(toField, sipEnd, to);
			LOG4CXX_DEBUG(s_sipExtractionLog, "to: " + to);

			GrabSipName(toField, toFieldEnd, info->m_toName);

			char* sipUser = memFindAfter("sip:", toField, toFieldEnd);
			if(sipUser)
			{
				if(DLLCONFIG.m_sipReportFullAddress)
				{
					GrabSipUserAddress(sipUser, toFieldEnd, info->m_to);
				}
				else
				{
					GrabSipUriUser(sipUser, toFieldEnd, info->m_to);
				}
				GrabSipUriDomain(sipUser, toFieldEnd, info->m_toDomain);
			}
			else
			{
				if(DLLCONFIG.m_sipReportFullAddress)
				{
					GrabSipUserAddress(toField, toFieldEnd, info->m_to);
				}
				else
				{
					GrabSipUriUser(toField, toFieldEnd, info->m_to);
				}
				GrabSipUriDomain(toField, toFieldEnd, info->m_toDomain);
			}
		}
		if(contactField)
		{
			CStdString contact;
			char* contactFieldEnd = GrabLine(contactField, sipEnd, contact);
			LOG4CXX_DEBUG(s_sipExtractionLog, "contact: " + contact);

			GrabSipName(contactField, contactFieldEnd, info->m_contactName);

			char* sipUser = memFindAfter("sip:", contactField, contactFieldEnd);
			if(sipUser)
			{
				if(DLLCONFIG.m_sipReportFullAddress)
				{
					GrabSipUserAddress(sipUser, contactFieldEnd, info->m_contact);
				}
				else
				{
					GrabSipUriUser(sipUser, contactFieldEnd, info->m_contact);
				}
				GrabSipUriDomain(sipUser, contactFieldEnd, info->m_contactDomain);
			}
			else
			{
				if(DLLCONFIG.m_sipReportFullAddress)
				{
					GrabSipUserAddress(contactField, contactFieldEnd, info->m_contact);
				}
				else
				{
					GrabSipUriUser(contactField, contactFieldEnd, info->m_contact);
				}
				GrabSipUriDomain(contactField, contactFieldEnd, info->m_contactDomain);
			}

		}
		if(callIdField)
		{
			GrabTokenSkipLeadingWhitespaces(callIdField, sipEnd, info->m_callId);
		}

		info->m_senderIp = ipHeader->ip_src;
		info->m_receiverIp = ipHeader->ip_dest;

		if(!info->m_callId.size())
		{
			drop = true;
		}
		if(!info->m_contact)
		{
			drop = true;
		}

		CStdString logMsg;
		info->ToString(logMsg);
		logMsg = sipMethod + ": " + logMsg;
		LOG4CXX_INFO(s_sipPacketLog, logMsg);

		if(drop == false)
		{
			VoIpSessionsSingleton::instance()->ReportSip302MovedTemporarily(info);
		}
		else
		{
			CStdString packetInfo;

			info->ToString(packetInfo);
			logMsg.Format("Dropped this %s: %s", sipMethod, packetInfo);
			LOG4CXX_INFO(s_sipPacketLog, logMsg);
		}
	}

	return result;
}

bool TrySipSubscribe(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, UdpHeaderStruct* udpHeader, u_char* udpPayload, u_char* packetEnd)
{
	bool result = false;
	CStdString sipMethod;
	SipSubscribeInfoRef info(new SipSubscribeInfo());

	int sipLength = ntohs(udpHeader->len) - sizeof(UdpHeaderStruct);
	char* sipEnd = (char*)udpPayload + sipLength;
	if(memcmp(SIP_METHOD_SUBSCRIBE, (void*)udpPayload, SIP_METHOD_SUBSCRIBE_SIZE) == 0)
	{
		sipMethod = SIP_METHOD_SUBSCRIBE;
		result = true;

		char* eventField = memFindAfter("Event:", (char*)udpPayload, sipEnd);

		if(eventField)
		{
			GrabTokenSkipLeadingWhitespaces(eventField, sipEnd, info->m_event);
			LOG4CXX_DEBUG(s_sipExtractionLog, "SIP SUBSCRIBE detected, Event:" + info->m_event);
		}

		char* callIdField = memFindAfter("Call-ID:", (char*)udpPayload, sipEnd);
		if(!callIdField)
		{
			callIdField = memFindAfter("\ni:", (char*)udpPayload, sipEnd);
		}
		if(callIdField)
		{
			GrabTokenSkipLeadingWhitespaces(callIdField, sipEnd, info->m_callId);
			LOG4CXX_DEBUG(s_sipExtractionLog,  "SIP SUBSCRIBE callId:" + info->m_callId);
		}
		//For now, we only concern if SIP SUBSCRIBE is of Sip Call Pick Up Service, otherwise just ignore it
		if(info->m_event.CompareNoCase("pickup") == 0)
		{
			VoIpSessionsSingleton::instance()->ReportSipSubscribe(info);
		}

	}

	return result;

}
bool TrySipInvite(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, UdpHeaderStruct* udpHeader, u_char* udpPayload, u_char* packetEnd)
{
	CStdString logMsg;
	bool result = false;
	bool drop = false;
	CStdString sipMethod;

	int sipLength = ntohs(udpHeader->len) - sizeof(UdpHeaderStruct);
	char* sipEnd = (char*)udpPayload + sipLength;

	if (DLLCONFIG.m_ipFragmentsReassemble == false && sipEnd > (char*)packetEnd && ipHeader->offset() == 0) {
		FLOG_DEBUG(s_sipExtractionLog, "Will try to process incomplete first fragment with id:%u",ipHeader->packetId());
		sipEnd = (char*)packetEnd;
	}

	if(sipLength < 3 || sipEnd > (char*)packetEnd)
	{
		drop = true;	// packet too short
	}
	else if(memcmp(SIP_METHOD_INVITE, (void*)udpPayload, SIP_METHOD_INVITE_SIZE) == 0)
	{
		sipMethod = SIP_METHOD_INVITE;
	}
	else if(memcmp(SIP_METHOD_ACK, (void*)udpPayload, SIP_METHOD_ACK_SIZE) == 0)
	{
		sipMethod = SIP_METHOD_ACK;
	}
	else if((DLLCONFIG.m_sipTreat200OkAsInvite == true) && (memcmp(SIP_RESPONSE_200_OK, (void*)udpPayload, SIP_RESPONSE_200_OK_SIZE) == 0))
	{
		sipMethod = SIP_METHOD_200_OK;
		LOG4CXX_DEBUG(s_sipExtractionLog, "TrySipInvite: packet matches 200 OK and SipTreat200OkAsInvite is enabled");
	}
	else if((DLLCONFIG.m_sipDetectSessionProgress == true) && (memcmp(SIP_RESPONSE_SESSION_PROGRESS, (void*)udpPayload, SIP_RESPONSE_SESSION_PROGRESS_SIZE) == 0))
	{
		sipMethod = SIP_RESPONSE_SESSION_PROGRESS;
	}
	else
	{
		drop = true;
	}

	if (drop == false)
	{
		result = true;
		std::array<const char*, eMaxSearchFieldPosition > sipFieldPointersArray = { 0 };

		SipInviteInfoRef info(new SipInviteInfo());
		info->m_sipMethod = sipMethod;
		char* fromField = memFindAfter("From:", (char*)udpPayload, sipEnd);
		if(!fromField)
		{
			fromField = memFindAfter("\nf:", (char*)udpPayload, sipEnd);
		}
		sipFieldPointersArray[eSearch_FieldFrom_Position] = fromField;
		char* toField = memFindAfter("To:", (char*)udpPayload, sipEnd);
		if(!toField)
		{
			toField = memFindAfter("\nt:", (char*)udpPayload, sipEnd);
		}
		sipFieldPointersArray[eSearch_FieldTo_Position] = toField;
		char* callIdField = memFindAfter("Call-ID:", (char*)udpPayload, sipEnd);
		if(!callIdField)
		{
			callIdField = memFindAfter("\ni:", (char*)udpPayload, sipEnd);
		}

		char* replacesField = memFindAfter("Replaces:", (char*)udpPayload, sipEnd);
		if(!replacesField)
		{
			replacesField = memFindAfter("\nr:", (char*)udpPayload, sipEnd);
		}
		sipFieldPointersArray[eSearch_FieldReplaces_Position] = replacesField;

		char * dialedNumber = NULL;
		if(!(DLLCONFIG.m_sipDialedNumberFieldName.length()==0) )
		{
			dialedNumber = memFindAfter(DLLCONFIG.m_sipDialedNumberFieldName + ":", (char*)udpPayload,sipEnd);
		}

		char * sipRemoteParty = NULL;
		if(!(DLLCONFIG.m_sipRemotePartyFieldName.length()==0) )
		{
			sipRemoteParty = memFindAfter(DLLCONFIG.m_sipRemotePartyFieldName + ":", (char*)udpPayload,sipEnd);
		}

		char* contactField = memFindAfter("Contact:", (char*)udpPayload, sipEnd);
		if(!contactField)
		{
			contactField = memFindAfter("\nc:", (char*)udpPayload, sipEnd);
		}
		sipFieldPointersArray[eSearch_FieldContact_Position] = replacesField;

                char * audioSdpStart = (char*) udpPayload;;
                char * audioSdpEnd   = (char*) sipEnd;;

                char* audioStart = memFindAfter("m=audio", (char*)udpPayload, sipEnd);
                char* videoStart = memFindAfter("m=video", (char*)udpPayload, sipEnd);

                if (audioStart < videoStart) {
                    audioSdpEnd = videoStart;
                }

                if (audioStart > videoStart) {
                    audioSdpStart = audioStart;
                }

		char* localExtensionField = memFindAfter(DLLCONFIG.m_sipLocalPartyFieldName, (char*)udpPayload, sipEnd);
		char* audioField = NULL;
		char* connectionAddressField = NULL;
		char* attribSendonly = memFindAfter("a=sendonly", (char*)audioSdpStart, audioSdpEnd);
		char* attribInactive = memFindAfter("a=inactive", (char*)audioSdpStart, audioSdpEnd);
		char* rtpmapAttribute = memFindAfter("\na=rtpmap:", (char*)audioSdpStart, audioSdpEnd);
		char* userAgentField = memFindAfter("\nUser-Agent:", (char*)udpPayload, sipEnd);

		if(DLLCONFIG.m_sipRequestUriAsLocalParty == true)
		{
			char* sipUriAttribute = memFindAfter("INVITE ", (char*)udpPayload, sipEnd);

			if(sipUriAttribute)
			{
				if(s_sipExtractionLog->isDebugEnabled())
				{
					CStdString uri;
					GrabLine(sipUriAttribute, sipEnd, uri);
					LOG4CXX_DEBUG(s_sipExtractionLog, "uri: " + uri);
				}

				char* sipUriAttributeEnd = memFindEOL(sipUriAttribute, sipEnd);
				char* sipUser = memFindAfter("sip:", sipUriAttribute, sipUriAttributeEnd);

				if(sipUser)
				{
					if(DLLCONFIG.m_sipReportFullAddress)
					{
						GrabSipUserAddress(sipUser, sipUriAttributeEnd, info->m_requestUri);
					}
					else
					{
						GrabSipUriUser(sipUser, sipUriAttributeEnd, info->m_requestUri);
					}
				}
				else
				{
					if(DLLCONFIG.m_sipReportFullAddress)
					{
						GrabSipUserAddress(sipUriAttribute, sipUriAttributeEnd, info->m_requestUri);
					}
					else
					{
						GrabSipUriUser(sipUriAttribute, sipUriAttributeEnd, info->m_requestUri);
					}
				}

				if(s_sipExtractionLog->isDebugEnabled())
				{
					LOG4CXX_DEBUG(s_sipExtractionLog, "extracted uri: " + info->m_requestUri);
				}
			}
		}

		if(fromField)
		{
			if(s_sipExtractionLog->isDebugEnabled())
			{
				CStdString from;
				GrabLine(fromField, sipEnd, from);
				LOG4CXX_DEBUG(s_sipExtractionLog, "from: " + from);
			}

			char* fromFieldEnd = memFindEOL(fromField, sipEnd);

			GrabSipName(fromField, fromFieldEnd, info->m_fromName);

			char* sipUser = memFindAfter("sip:", fromField, fromFieldEnd);
			if(sipUser)
			{
				if(DLLCONFIG.m_sipReportFullAddress)
				{
					GrabSipUserAddress(sipUser, fromFieldEnd, info->m_from);
				}
				else
				{
					GrabSipUriUser(sipUser, fromFieldEnd, info->m_from);
				}
				GrabSipUriDomain(sipUser, fromFieldEnd, info->m_fromDomain);
			}
			else
			{
				if(DLLCONFIG.m_sipReportFullAddress)
				{
					GrabSipUserAddress(fromField, fromFieldEnd, info->m_from);
				}
				else
				{
					GrabSipUriUser(fromField, fromFieldEnd, info->m_from);
				}
				GrabSipUriDomain(fromField, fromFieldEnd, info->m_fromDomain);
			}
		}
		if(toField)
		{
			CStdString to;
			char* toFieldEnd = GrabLine(toField, sipEnd, to);
			LOG4CXX_DEBUG(s_sipExtractionLog, "to: " + to);

			GrabSipName(toField, toFieldEnd, info->m_toName);

			char* sipUser = memFindAfter("sip:", toField, toFieldEnd);
			if(!sipUser)
			{
				sipUser = memFindAfter("tel:", toField, toFieldEnd);
			}
			if(sipUser)
			{
				if(DLLCONFIG.m_sipReportFullAddress)
				{
					GrabSipUserAddress(sipUser, toFieldEnd, info->m_to);
				}
				else
				{
					GrabSipUriUser(sipUser, toFieldEnd, info->m_to);
				}
				GrabSipUriDomain(sipUser, toFieldEnd, info->m_toDomain);
			}
			else
			{
				if(DLLCONFIG.m_sipReportFullAddress)
				{
					GrabSipUserAddress(toField, toFieldEnd, info->m_to);
				}
				else
				{
					GrabSipUriUser(toField, toFieldEnd, info->m_to);
				}
				GrabSipUriDomain(toField, toFieldEnd, info->m_toDomain);
			}
			if(DLLCONFIG.m_sipGroupPickUpPattern == info->m_to)
			{
				info->m_SipGroupPickUpPatternDetected = true;
			}
		}
		if(dialedNumber)
		{
			CStdString token;
			GrabTokenSkipLeadingWhitespaces(dialedNumber, sipEnd, token);
			info->m_sipDialedNumber = token;
		}
		if(sipRemoteParty)
		{
			CStdString token;

			char* sip = memFindAfter("sip:", sipRemoteParty, sipEnd);
			if(sip)
			{
				if(DLLCONFIG.m_sipReportFullAddress)
				{
					GrabSipUserAddress(sip, sipEnd, token);
				}
				else
				{
					GrabSipUriUser(sip, sipEnd, token);
				}
			}
			else
			{
				if(DLLCONFIG.m_sipReportFullAddress)
				{
					GrabSipUserAddress(sipRemoteParty, sipEnd, token);
				}
				else
				{
					GrabSipUriUser(sipRemoteParty, sipEnd, token);
				}
			}
			info->m_sipRemoteParty = token;
		}
		if(callIdField)
		{
			GrabTokenSkipLeadingWhitespaces(callIdField, sipEnd, info->m_callId);
			audioField = memFindAfter("m=audio ", callIdField, sipEnd);
			connectionAddressField = memFindAfter("c=IN IP4 ", callIdField, sipEnd);
		}
		if(replacesField)
		{
			CStdString fieldContent;
			GrabTokenSkipLeadingWhitespaces(replacesField, sipEnd, fieldContent);
			int firstsemicoma;
			firstsemicoma = fieldContent.Find(';');
			if(firstsemicoma != -1)
			{
				info->m_replacesId = fieldContent.substr(0, firstsemicoma);
			}

			LOG4CXX_DEBUG(s_sipExtractionLog, "replaces CallId:" + info->m_replacesId);
		}
		if(localExtensionField)
		{
			CStdString localExtension;
			GrabTokenSkipLeadingWhitespaces(localExtensionField, sipEnd, localExtension);
			if(localExtension.size() > 0)
			{
				info->m_from = localExtension;
			}
		}
		if(userAgentField)
		{
			GrabTokenSkipLeadingWhitespaces(userAgentField, sipEnd, info->m_userAgent);
		}
		if(audioField)
		{
			GrabToken(audioField, sipEnd, info->m_fromRtpPort);
		}
		if(attribSendonly || attribInactive)
		{
			info->m_attrSendonly = true;
		}
		if(connectionAddressField)
		{
			CStdString connectionAddress;
			GrabToken(connectionAddressField, sipEnd, connectionAddress);
			struct in_addr fromIp;
			if(connectionAddress.size())
			{
				if(inet_pton4((PCSTR)connectionAddress, &fromIp))
				{
					info->m_fromRtpIp = fromIp;

					if (DLLCONFIG.m_sipDropIndirectInvite)
					{
						if((unsigned int)fromIp.s_addr != (unsigned int)ipHeader->ip_src.s_addr)
						{
							// SIP invite SDP connection address does not match with SIP packet origin
							drop =true;
						}
					}
				}
			}
		}
		if(contactField && sipMethod == SIP_METHOD_INVITE)
		{
			CStdString contact;
			char* contactFieldEnd = GrabLine(contactField, sipEnd, contact);
			LOG4CXX_DEBUG(s_sipExtractionLog, "contact: " + contact);

			GrabSipName(contactField, contactFieldEnd, info->m_contactName);

			char* sipUser = memFindAfter("sip:", contactField, contactFieldEnd);
			if(sipUser)
			{
				if(DLLCONFIG.m_sipReportFullAddress)
				{
					GrabSipUserAddress(sipUser, contactFieldEnd, info->m_contact);
				}
				else
				{
					GrabSipUriUser(sipUser, contactFieldEnd, info->m_contact);
				}
				GrabSipUriDomain(sipUser, contactFieldEnd, info->m_contactDomain);
			}
			else
			{
				if(DLLCONFIG.m_sipReportFullAddress)
				{
					GrabSipUserAddress(contactField, contactFieldEnd, info->m_contact);
				}
				else
				{
					GrabSipUriUser(contactField, contactFieldEnd, info->m_contact);
				}
				GrabSipUriDomain(contactField, contactFieldEnd, info->m_contactDomain);
			}
		}
		// SIP fields extraction
		for(std::list<CStdString>::iterator it = DLLCONFIG.m_sipExtractFields.begin(); it != DLLCONFIG.m_sipExtractFields.end(); it++)
		{
			CStdString fieldName = *it + ":";
			char* szField = memFindAfter((PSTR)(PCSTR)fieldName, (char*)udpPayload, sipEnd);
			if(szField)
			{
				CStdString field;

				// XXX
				// The line below was replaced because I experienced
				// cases where we would have a leading whitespace in the
				// tag which has been extracted.  However, since we are
				// dealing with SIP, RFC 3261, section 7.3.1 illustrates
				// that any leading whitespaces after the colon is not
				// in fact part of the header value.  Therefore, I
				// created the GrabLineSkipLeadingWhitespace() function
				// which I use in this particular case.
				//
				// Hope this is ok.
				//
				// --Gerald
				//
				//GrabLine(szField, sipEnd, field);

				GrabLineSkipLeadingWhitespace(szField, sipEnd, field);
				info->m_extractedFields.insert(std::make_pair(*it, field));
			}
		}
        
		ExtractKeyValuesData(info->m_extractedFields, sipFieldPointersArray, reinterpret_cast<char *> (udpPayload), sipLength);
        
		if(DLLCONFIG.m_rtpReportDtmf)
		{
			if(rtpmapAttribute)
			{
				CStdString rtpPayloadType, nextToken;
				char *nextStep = NULL;

				while(rtpmapAttribute && rtpmapAttribute < sipEnd)
				{
					rtpPayloadType = "";
					GrabTokenSkipLeadingWhitespaces(rtpmapAttribute, audioSdpEnd, rtpPayloadType);
					nextToken.Format("%s ", rtpPayloadType);
					nextStep = memFindAfter((char*)nextToken.c_str(), rtpmapAttribute, audioSdpEnd);

					/* We need our "nextStep" to contain at least the length
					 * of the string "telephone-event", 15 characters */
					if(nextStep && ((sipEnd - nextStep) >= 15))
					{
						if(strncasecmp(nextStep, "telephone-event", 15) == 0) {
							/* Our DTMF packets are indicated using * the payload type rtpPayloadType */
							info->m_telephoneEventPayloadType = StringToInt(rtpPayloadType);
							break;
						}
					}

					rtpmapAttribute = memFindAfter("\na=rtpmap:", rtpmapAttribute, audioSdpEnd);
				}
			}
		}

		rtpmapAttribute = memFindAfter("\na=rtpmap:", (char*)audioSdpStart, audioSdpEnd);
		if(rtpmapAttribute)
		{
			GetDynamicPayloadMapping(audioSdpStart,audioSdpEnd, info->m_orekaRtpPayloadTypeMap);
			if(s_sipExtractionLog->isDebugEnabled())
			{
				logMsg.Format("%s: [%s]: Re-mapped codecs", sipMethod, info->m_callId);
				for (int i = 0; i < 32; i++)
				{
					if (info->m_orekaRtpPayloadTypeMap[i])
					{
						logMsg.AppendFormat(" %d->%d", i+96, info->m_orekaRtpPayloadTypeMap[i]);
					}
				}
				LOG4CXX_DEBUG(s_sipExtractionLog, logMsg);
			}
		}

		if((unsigned int)info->m_fromRtpIp.s_addr == 0)
		{
			// In case connection address could not be extracted, use SIP invite sender IP address
			if(DLLCONFIG.m_dahdiIntercept == true)
			{
				info->m_fromRtpIp = ipHeader->ip_dest;
			}
			else
			{
				info->m_fromRtpIp = ipHeader->ip_src;
			}
		}
		if(sipMethod == SIP_METHOD_200_OK)
		{
			info->m_senderIp = ipHeader->ip_dest;
			info->m_receiverIp = ipHeader->ip_src;
		}
		else
		{
			info->m_senderIp = ipHeader->ip_src;
			info->m_receiverIp = ipHeader->ip_dest;
		}
		info->m_originalSenderIp = ipHeader->ip_src;
		info->m_recvTime = time(NULL);
		memcpy(info->m_senderMac, ethernetHeader->sourceMac, sizeof(info->m_senderMac));
		memcpy(info->m_receiverMac, ethernetHeader->destinationMac, sizeof(info->m_receiverMac));

		if(sipMethod.Equals(SIP_METHOD_INVITE) || info->m_fromRtpPort.size())
		{
			// Only log SIP non-INVITE messages that contain SDP (i.e. with a valid RTP port)
			info->ToString(logMsg);
			logMsg = sipMethod + ": " + logMsg;
			LOG4CXX_INFO(s_sipPacketLog, logMsg);
		}

		//Sip INVITE without sdp will be reported, but other methods without sdp will not be
		if(drop == false && sipMethod == SIP_METHOD_INVITE && info->m_from.size() && info->m_to.size() && info->m_callId.size())
		{
			VoIpSessionsSingleton::instance()->ReportSipInvite(info);
		}
		else if(drop == false && info->m_fromRtpPort.size() && info->m_from.size() && info->m_to.size() && info->m_callId.size())
		{
			VoIpSessionsSingleton::instance()->ReportSipInvite(info);
		}
	}
	return result;
}

bool TrySipRefer(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, UdpHeaderStruct* udpHeader, u_char* udpPayload, u_char* packetEnd)
{
	bool result = false;

	int sipLength = ntohs(udpHeader->len) - sizeof(UdpHeaderStruct);
	char* sipEnd = (char*)udpPayload + sipLength;
	if(sipLength < SIP_METHOD_REFER_SIZE || sipEnd > (char*)packetEnd)
	{
		;	// packet too short
	}
	else if(memcmp(SIP_METHOD_REFER, (void*)udpPayload, SIP_METHOD_REFER_SIZE) == 0)
	{
		result = true;

		SipReferRef info(new SipRefer());
		info->m_timestamp = time(NULL);
		char* referToField = memFindAfter("Refer-To:", (char*)udpPayload, sipEnd);
		char* referredByField = memFindAfter("Referred-By:", (char*)udpPayload, sipEnd);
		char* callIdField = memFindAfter("Call-ID:", (char*)udpPayload, sipEnd);
		if(!callIdField)
		{
			callIdField = memFindAfter("\ni:", (char*)udpPayload, sipEnd);
		}
		char* fromField = memFindAfter("From:", (char*)udpPayload, sipEnd);
		if(!fromField)
		{
			fromField = memFindAfter("\nf:", (char*)udpPayload, sipEnd);
		}
		char* toField = memFindAfter("To:", (char*)udpPayload, sipEnd);
		if(!toField)
		{
			toField = memFindAfter("\nt:", (char*)udpPayload, sipEnd);
		}

		if(callIdField)
		{
			GrabTokenSkipLeadingWhitespaces(callIdField, sipEnd, info->m_callId);
		}

		if(fromField)
		{
			if(s_sipExtractionLog->isDebugEnabled())
			{
				CStdString from;
				GrabLine(fromField, sipEnd, from);
				LOG4CXX_DEBUG(s_sipExtractionLog, "from: " + from);
			}
			char* fromFieldEnd = memFindEOL(fromField, sipEnd);
			GrabSipName(fromField, fromFieldEnd, info->m_fromName);
			char* sipUser = memFindAfter("sip:", fromField, fromFieldEnd);
			if(sipUser)
			{
				GrabSipUriUser(sipUser, fromFieldEnd, info->m_from);
				GrabSipUriDomain(sipUser, fromFieldEnd, info->m_fromDomain);
			}
			else
			{
				GrabSipUriUser(fromField, fromFieldEnd, info->m_from);
				GrabSipUriDomain(fromField, fromFieldEnd, info->m_fromDomain);
			}
		}
		if(toField)
		{
			CStdString to;
			char* toFieldEnd = GrabLine(toField, sipEnd, to);
			LOG4CXX_DEBUG(s_sipExtractionLog, "to: " + to);
			GrabSipName(toField, toFieldEnd, info->m_toName);
			char* sipUser = memFindAfter("sip:", toField, toFieldEnd);
			if(sipUser)
			{
				GrabSipUriUser(sipUser, toFieldEnd, info->m_to);
				GrabSipUriDomain(sipUser, toFieldEnd, info->m_toDomain);
			}
			else
			{
				GrabSipUriUser(toField, toFieldEnd, info->m_to);
				GrabSipUriDomain(toField, toFieldEnd, info->m_toDomain);
			}
		}

		if(referToField)
		{
			char* referToFieldEnd = memFindEOL(referToField, sipEnd);
			char* sipUser = memFindAfter("sip:", referToField, referToFieldEnd);
			if(sipUser)
			{
				GrabSipUriUser(sipUser, referToFieldEnd, info->m_referToParty);

			}
		}
		if(referredByField)
		{
			char* referredByFieldEnd = memFindEOL(referredByField, sipEnd);
			char* sipUser = memFindAfter("sip:", referredByField, referredByFieldEnd);
			if(sipUser)
			{
				GrabSipUriUser(sipUser, referredByFieldEnd, info->m_referredByParty);

			}
		}
		if((!(info->m_to.length()==0)) && (info->m_to.CompareNoCase(info->m_referToParty) != 0) && (info->m_to.CompareNoCase(info->m_referredByParty) != 0))
		{
			info->m_referredParty = info->m_to;
		}
		if((!(info->m_from.length()==0)) && (info->m_from.CompareNoCase(info->m_referToParty) != 0) && (info->m_from.CompareNoCase(info->m_referredByParty) != 0))
		{
			info->m_referredParty = info->m_from;
		}
		info->m_senderIp = ipHeader->ip_src;
		info->m_receiverIp = ipHeader->ip_dest;

		CStdString logMsg;

		info->ToString(logMsg);
		logMsg = "REFER: " + logMsg;
		LOG4CXX_INFO(s_sipPacketLog, logMsg);

		VoIpSessionsSingleton::instance()->ReportSipRefer(info);
	}
	return result;
}



