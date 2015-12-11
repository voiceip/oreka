/*
 * Oreka -- A media capture and retrieval platform
 *
 * Copyright (C) 2005, orecx LLC
 *
 * http://www.orecx.com
 *
 */
#include "SkinnyParsers.h"
#include "ParsingUtils.h"
#include "LogManager.h"
#include "VoIpConfig.h"
#include "Win1251.h"

static LoggerPtr s_skinnyParsersLog = Logger::getLogger("packet.skinnyparsers");
extern unsigned short utf[256];		//UTF-8 encoding table (partial)


void ScanAllSkinnyMessages(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, TcpHeaderStruct* tcpHeader, u_char* ipPacketEnd)
{
	u_char* startTcpPayload = (u_char*)tcpHeader + (tcpHeader->off * 4);
	SkinnyHeaderStruct* skinnyHeader = (SkinnyHeaderStruct*)(startTcpPayload);

	// Scan all skinny messages in this TCP packet
	while(	ipPacketEnd > (u_char*)skinnyHeader &&
			(u_char*)skinnyHeader>=startTcpPayload &&
			(ipPacketEnd - (u_char*)skinnyHeader) > SKINNY_MIN_MESSAGE_SIZE	&&
			skinnyHeader->len > 1 && skinnyHeader->len < 2048 &&
			skinnyHeader->messageType >= 0x0 && skinnyHeader->messageType <= 0x200 )	// Last known skinny message by ethereal is 0x13F, but seen higher message ids in the field.
	{
		if(s_skinnyParsersLog->isDebugEnabled())
		{
			CStdString dbg;
			unsigned int offset = (u_char*)skinnyHeader - startTcpPayload;
			dbg.Format("Offset:%x Len:%u Type:%x %s", offset, skinnyHeader->len, skinnyHeader->messageType, SkinnyMessageToString(skinnyHeader->messageType));
			LOG4CXX_DEBUG(s_skinnyParsersLog, dbg);
		}

		HandleSkinnyMessage(skinnyHeader, ipHeader, ipPacketEnd, tcpHeader);

		// Point to next skinny message within this TCP packet
		skinnyHeader = (SkinnyHeaderStruct*)((u_char*)skinnyHeader + SKINNY_HEADER_LENGTH + skinnyHeader->len);
	}
}

void HandleSkinnyMessage(SkinnyHeaderStruct* skinnyHeader, IpHeaderStruct* ipHeader, u_char* packetEnd, TcpHeaderStruct* tcpHeader)
{
	bool useful = true;
	CStdString logMsg;

	SkStartMediaTransmissionStruct* startMedia;
	SkStartMediaTransmissionStruct smtmp;
	SkStopMediaTransmissionStruct* stopMedia;
	SkCallInfoStruct* callInfo;
	SkCallStateMessageStruct* callStateMessage;
	SkOpenReceiveChannelAckStruct* openReceiveAck;
	SkOpenReceiveChannelAckStruct orcatmp;
	SkLineStatStruct* lineStat;
	SkCcm5CallInfoStruct* ccm5CallInfo;
	SkSoftKeyEventMessageStruct* softKeyEvent;
	SkSoftKeySetDescriptionStruct* softKeySetDescription;

	char szEndpointIp[16];
	char szCmIp[16];
	struct in_addr endpointIp = ipHeader->ip_dest;	// most of the interesting skinny messages are CCM -> phone
	struct in_addr cmIp = ipHeader->ip_src;
	unsigned short endpointTcpPort = ntohs(tcpHeader->dest);
	unsigned short cmTcpPort = ntohs(tcpHeader->source);


	memset(&smtmp, 0, sizeof(smtmp));
	memset(&orcatmp, 0, sizeof(orcatmp));

	switch(skinnyHeader->messageType)
	{
	case SkStartMediaTransmission:

		startMedia = (SkStartMediaTransmissionStruct*)skinnyHeader;
		SkCcm7_1StartMediaTransmissionStruct *ccm7_1sm;
		ccm7_1sm = (SkCcm7_1StartMediaTransmissionStruct*)skinnyHeader;

		if(SkinnyValidateStartMediaTransmission(startMedia, packetEnd))
		{
			if(s_skinnyParsersLog->isInfoEnabled())
			{
				char szRemoteIp[16];
				ACE_OS::inet_ntop(AF_INET, (void*)&startMedia->remoteIpAddr, szRemoteIp, sizeof(szRemoteIp));
				logMsg.Format(" CallId:%u PassThru:%u media address:%s,%u", startMedia->conferenceId, startMedia->passThruPartyId, szRemoteIp, startMedia->remoteTcpPort);
			}
			VoIpSessionsSingleton::instance()->ReportSkinnyStartMediaTransmission(startMedia, ipHeader, tcpHeader);
		}
		else if(SkinnyValidateCcm7_1StartMediaTransmission(ccm7_1sm, packetEnd))
		{
			startMedia = &smtmp;

			memcpy(&startMedia->header, &ccm7_1sm->header, sizeof(startMedia->header));
			startMedia->conferenceId = ccm7_1sm->conferenceId;
			startMedia->passThruPartyId = ccm7_1sm->passThruPartyId;
			memcpy(&startMedia->remoteIpAddr, &ccm7_1sm->remoteIpAddr, sizeof(startMedia->remoteIpAddr));
			startMedia->remoteTcpPort = ccm7_1sm->remoteTcpPort;

			if(s_skinnyParsersLog->isInfoEnabled())
			{
				char szRemoteIp[16];
				ACE_OS::inet_ntop(AF_INET, (void*)&startMedia->remoteIpAddr, szRemoteIp, sizeof(szRemoteIp));
				logMsg.Format(" (CCM 7.1) CallId:%u PassThru:%u media address:%s,%u", startMedia->conferenceId, startMedia->passThruPartyId, szRemoteIp, startMedia->remoteTcpPort);
			}

			VoIpSessionsSingleton::instance()->ReportSkinnyStartMediaTransmission(startMedia, ipHeader, tcpHeader);
		}
		else
		{
			useful = false;
			LOG4CXX_WARN(s_skinnyParsersLog, "Invalid StartMediaTransmission.");
		}
		break;
	case SkStopMediaTransmission:
	case SkCloseReceiveChannel:
		// StopMediaTransmission and CloseReceiveChannel have the same definition, treat them the same for now.
		stopMedia = (SkStopMediaTransmissionStruct*)skinnyHeader;
		if(SkinnyValidateStopMediaTransmission(stopMedia, packetEnd))
		{
			if(s_skinnyParsersLog->isInfoEnabled())
			{
				logMsg.Format(" ConferenceId:%u PassThruPartyId:%u", stopMedia->conferenceId, stopMedia->passThruPartyId);
			}
			VoIpSessionsSingleton::instance()->ReportSkinnyStopMediaTransmission(stopMedia, ipHeader, tcpHeader);
		}
		else
		{
			useful = false;
			LOG4CXX_WARN(s_skinnyParsersLog, "Invalid StopMediaTransmission or CloseReceiveChannel.");
		}
		break;
	case SkCallInfoMessage:
		callInfo = (SkCallInfoStruct*)skinnyHeader;
		if(SkinnyValidateCallInfo(callInfo, packetEnd))
		{
			if(s_skinnyParsersLog->isInfoEnabled())
			{
				ConvertWin1251ToUtf8(callInfo->callingPartyName, utf);
				ConvertWin1251ToUtf8(callInfo->calledPartyName, utf);
				logMsg.Format(" CallId:%u calling:%s called:%s callingname:%s calledname:%s line:%d callType:%d",
								callInfo->callId, callInfo->callingParty, callInfo->calledParty,
								callInfo->callingPartyName, callInfo->calledPartyName, callInfo->lineInstance, callInfo->callType);

			}
			VoIpSessionsSingleton::instance()->ReportSkinnyCallInfo(callInfo, ipHeader, tcpHeader);
		}
		else
		{
			useful = false;
			LOG4CXX_WARN(s_skinnyParsersLog, "Invalid CallInfoMessage.");
		}
		break;
	case SkCallStateMessage:
		callStateMessage = (SkCallStateMessageStruct*)skinnyHeader;
		if(SkinyValidateCallStateMessage(callStateMessage, packetEnd))
		{
			VoIpSessionsSingleton::instance()->ReportSkinnyCallStateMessage(callStateMessage, ipHeader, tcpHeader);
		}
	case SkCcm5CallInfoMessage:
		ccm5CallInfo = (SkCcm5CallInfoStruct*)skinnyHeader;
		if(SkinnyValidateCcm5CallInfo(ccm5CallInfo, packetEnd))
		{
			// Extract Calling and Called number.
			char* parties = (char*)(&ccm5CallInfo->parties);
			char* partiesPtr = parties;
			long partiesLen = (long)packetEnd - (long)ccm5CallInfo - sizeof(SkCcm5CallInfoStruct);

			CStdString callingParty;
			CStdString calledParty;
			CStdString callingPartyName;
			CStdString calledPartyName;

			// Tokens separated by a single null char. Multiple sequential null chars result in empty tokens.
			// There are 12 tokens:
			// calling general number, called long num, called long num, called long num
			// calling extension (1), called ext num, called ext num, called ext num
			// calling name, called name (2), called name, called name
			//(1) not always available, in this case, use calling general number
			//(2) not always available
			int tokenNr = 0;
			while(tokenNr < 12 && partiesPtr < parties+partiesLen)
			{
				CStdString party;
				GrabTokenAcceptSpace(partiesPtr, parties+partiesLen, party);
				if(s_skinnyParsersLog->isDebugEnabled())
				{
					logMsg = logMsg + party + ", ";
				}
				partiesPtr += party.size() + 1;
				tokenNr += 1;

				switch(tokenNr)
				{
				case 1:
					// Token 1 can be the general office number for outbound
					// TODO, report this as local entry point
					callingParty = party;
					break;
				case 2:
					calledParty = party;
					break;
				case 3:
					if(DLLCONFIG.m_cucm7_1Mode == true)
					{
						// In CCM 7.1, it appears that each party is
						// named twice
						calledParty = party;
					}
					break;
				case 5:
					if(DLLCONFIG.m_cucm7_1Mode == false)
					{
						// Token 5 is the calling extension, use this if outbound for callingParty instead of general number
						// With CCM 7.1, this appears not to be the case
						if(party.size()>0 && ccm5CallInfo->callType == SKINNY_CALL_TYPE_OUTBOUND)
						{
							callingParty = party;
						}
					}
					break;
				case 6:
					// This is the called party extension, not used for now
					break;
				case 9:
					if(ccm5CallInfo->header.headerVersion == 0x0)
					{
						callingPartyName = party;
					}
					break;
				case 10:
					if(ccm5CallInfo->header.headerVersion == 0x0)
					{
						calledPartyName = party;
					}
					else
					{
						callingPartyName = party;
					}
					break;
				case 11:
					if(ccm5CallInfo->header.headerVersion != 0x0)
					{
						calledPartyName = party;
					}
					break;
				case 12:
					if(ccm5CallInfo->header.headerVersion != 0x0 && party.length() > calledPartyName.length())
					{
						calledPartyName = party;
					}
					break;
				}
			}
			LOG4CXX_DEBUG(s_skinnyParsersLog, "parties tokens:" + logMsg);

			// Emulate a regular CCM CallInfo message
			SkCallInfoStruct callInfo;
			strcpy(callInfo.calledParty, (PCSTR)calledParty);
			strcpy(callInfo.callingParty, (PCSTR)callingParty);
			callInfo.callId = ccm5CallInfo->callId;
			callInfo.callType = ccm5CallInfo->callType;
			callInfo.lineInstance = 0;
			if(calledPartyName.size())
			{
				strncpy(callInfo.calledPartyName, (PCSTR)calledPartyName, sizeof(callInfo.calledPartyName));
			}
			else
			{
				callInfo.calledPartyName[0] = '\0';
			}
			if(callingPartyName.size())
			{
				strncpy(callInfo.callingPartyName, (PCSTR)callingPartyName, sizeof(callInfo.callingPartyName));
			}
			else
			{
				callInfo.callingPartyName[0] = '\0';
			}

			if(s_skinnyParsersLog->isInfoEnabled())
			{
				logMsg.Format(" CallId:%u calling:%s called:%s callingname:%s calledname:%s callType:%d", callInfo.callId,
								callInfo.callingParty, callInfo.calledParty, callInfo.callingPartyName, callInfo.calledPartyName, callInfo.callType);
			}
			VoIpSessionsSingleton::instance()->ReportSkinnyCallInfo(&callInfo, ipHeader, tcpHeader);
		}
		break;
	case SkOpenReceiveChannelAck:

		openReceiveAck = (SkOpenReceiveChannelAckStruct*)skinnyHeader;
		SkCcm7_1SkOpenReceiveChannelAckStruct *orca;
		orca = (SkCcm7_1SkOpenReceiveChannelAckStruct*)skinnyHeader;

		if(SkinnyValidateOpenReceiveChannelAck(openReceiveAck, packetEnd))
		{
			if(s_skinnyParsersLog->isInfoEnabled())
			{
				char szMediaIp[16];
				ACE_OS::inet_ntop(AF_INET, (void*)&openReceiveAck->endpointIpAddr, szMediaIp, sizeof(szMediaIp));
				logMsg.Format(" PassThru:%u media address:%s,%u", openReceiveAck->passThruPartyId, szMediaIp, openReceiveAck->endpointTcpPort);
			}
			endpointIp = ipHeader->ip_src;	// this skinny message is phone -> CCM
			cmIp = ipHeader->ip_dest;
			endpointTcpPort = ntohs(tcpHeader->source);
			cmTcpPort = ntohs(tcpHeader->dest);
			VoIpSessionsSingleton::instance()->ReportSkinnyOpenReceiveChannelAck(openReceiveAck, ipHeader, tcpHeader);
		}
		else if(SkinnyValidateCcm7_1SkOpenReceiveChannelAckStruct(orca, packetEnd))
		{
			openReceiveAck = &orcatmp;

			memcpy(&openReceiveAck->header, &orca->header, sizeof(openReceiveAck->header));
			openReceiveAck->openReceiveChannelStatus = orca->openReceiveChannelStatus;
			memcpy(&openReceiveAck->endpointIpAddr, &orca->endpointIpAddr, sizeof(openReceiveAck->endpointIpAddr));
			openReceiveAck->endpointTcpPort = orca->endpointTcpPort;
			openReceiveAck->passThruPartyId = orca->passThruPartyId;

			if(s_skinnyParsersLog->isInfoEnabled())
			{
				char szMediaIp[16];
				ACE_OS::inet_ntop(AF_INET, (void*)&openReceiveAck->endpointIpAddr, szMediaIp, sizeof(szMediaIp));
				logMsg.Format(" (CCM 7.1) PassThru:%u media address:%s,%u", openReceiveAck->passThruPartyId, szMediaIp, openReceiveAck->endpointTcpPort);
			}
			endpointIp = ipHeader->ip_src;	// this skinny message is phone -> CCM
			cmIp = ipHeader->ip_dest;
			endpointTcpPort = ntohs(tcpHeader->source);
			cmTcpPort = ntohs(tcpHeader->dest);
			VoIpSessionsSingleton::instance()->ReportSkinnyOpenReceiveChannelAck(openReceiveAck, ipHeader, tcpHeader);
		}

		else
		{
			useful = false;
			LOG4CXX_WARN(s_skinnyParsersLog, "Invalid OpenReceiveChannelAck.");
		}
		break;
	case SkLineStatMessage:
		lineStat = (SkLineStatStruct*)skinnyHeader;
		if(SkinnyValidateLineStat(lineStat, packetEnd))
		{
			if(s_skinnyParsersLog->isInfoEnabled())
			{
				logMsg.Format(" line:%u extension:%s display name:%s", lineStat->lineNumber, lineStat->lineDirNumber, lineStat->displayName);
			}
			endpointIp = ipHeader->ip_dest;	// this skinny message is CCM -> phone
			VoIpSessionsSingleton::instance()->ReportSkinnyLineStat(lineStat, ipHeader, tcpHeader);
		}
		else
		{
			useful = false;
			LOG4CXX_WARN(s_skinnyParsersLog, "Invalid LineStatMessage.");
		}

		break;
	case SkSoftKeyEventMessage:
		softKeyEvent = (SkSoftKeyEventMessageStruct*)skinnyHeader;
		if(SkinnyValidateSoftKeyEvent(softKeyEvent, packetEnd))
		{
			useful = true;
			logMsg.Format(" eventString:%s eventNum:%d line:%lu callId:%lu",
					SoftKeyEvent::SoftKeyEventToString(softKeyEvent->softKeyEvent),
					softKeyEvent->softKeyEvent,
					softKeyEvent->lineInstance,
					softKeyEvent->callIdentifier);

			endpointIp = ipHeader->ip_src;  // this skinny message is phone -> CCM
			cmIp = ipHeader->ip_dest;
			endpointTcpPort = ntohs(tcpHeader->source);
			cmTcpPort = ntohs(tcpHeader->dest);

			switch(softKeyEvent->softKeyEvent)
			{
			case SoftKeyEvent::SkSoftKeyHold:
				VoIpSessionsSingleton::instance()->ReportSkinnySoftKeyHold(softKeyEvent, ipHeader, tcpHeader);
				break;
			case SoftKeyEvent::SkSoftKeyResume:
				VoIpSessionsSingleton::instance()->ReportSkinnySoftKeyResume(softKeyEvent, ipHeader, tcpHeader);
				break;
			case SoftKeyEvent::SkSoftKeyConfrn:
				if (DLLCONFIG.m_SkinnyTrackConferencesTransfers == true)
				{
					VoIpSessionsSingleton::instance()->ReportSkinnySoftKeyConfPressed(endpointIp, tcpHeader);
				}
				break;
			default:
				CStdString logSoftKey;

				logSoftKey.Format("Ignoring unsupported event %s (%d)",
					SoftKeyEvent::SoftKeyEventToString(softKeyEvent->softKeyEvent),
					softKeyEvent->softKeyEvent);
				LOG4CXX_INFO(s_skinnyParsersLog, logSoftKey);
				break;
			}
		}
		else
		{
			useful = false;
			LOG4CXX_WARN(s_skinnyParsersLog, "Invalid SoftKeyEventMessage.");
		}
		break;
	case SkSoftKeySetDescription:
		softKeySetDescription = (SkSoftKeySetDescriptionStruct*)skinnyHeader;
		if(SkinnyValidateSoftKeySetDescription(softKeySetDescription, packetEnd))
		{
			useful = true;
			logMsg.Format(" eventString:%s eventNum:%d line:%lu callId:%lu",
					SoftKeySetDescription::SoftKeySetDescriptionToString(softKeySetDescription->softKeySetDescription),
					softKeySetDescription->softKeySetDescription,
					softKeySetDescription->lineInstance,
					softKeySetDescription->callIdentifier);

			endpointIp = ipHeader->ip_dest;
			switch(softKeySetDescription->softKeySetDescription)
			{
				case SoftKeySetDescription::SkSoftKeySetConference:
					if (DLLCONFIG.m_SkinnyTrackConferencesTransfers == true)
					{
						VoIpSessionsSingleton::instance()->ReportSkinnySoftKeySetConfConnected(endpointIp, tcpHeader);
					}
					break;
				case SoftKeySetDescription::SkSoftKeySetTransfer:
					VoIpSessionsSingleton::instance()->ReportSkinnySoftKeySetTransfConnected(softKeySetDescription, ipHeader, tcpHeader);
					break;
			}
		}
		else
		{
			useful = false;
			LOG4CXX_WARN(s_skinnyParsersLog, "Invalid SoftKeySetDescription.");
		}
		break;
	default:
		useful = false;
	}
	if(useful && s_skinnyParsersLog->isInfoEnabled())
	{
		CStdString msg = SkinnyMessageToString(skinnyHeader->messageType);
		ACE_OS::inet_ntop(AF_INET, (void*)&endpointIp, szEndpointIp, sizeof(szEndpointIp));
		ACE_OS::inet_ntop(AF_INET, (void*)&cmIp, szCmIp, sizeof(szCmIp));
		logMsg.Format("processed %s%s endpoint:%s,%u cm:%s,%u", msg, logMsg, szEndpointIp, endpointTcpPort, szCmIp, cmTcpPort);
		LOG4CXX_INFO(s_skinnyParsersLog, logMsg);
	}
}
