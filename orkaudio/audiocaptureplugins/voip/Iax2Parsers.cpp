/*
 * Oreka -- A media capture and retrieval platform
 *
 * Copyright (C) 2005, orecx LLC
 *
 * http://www.orecx.com
 *
 */
#ifdef WIN32
#define snprintf _snprintf
#endif

#include "Iax2Parsers.h"
#include "VoIpConfig.h"

static LoggerPtr s_iax2parsersLog = Logger::getLogger("iax2parsers");

static int iax2_codec_to_rtp_payloadtype(int codec)
{
	switch(codec) {
	case IAX2_CODEC_ULAW:
		return 0;
	case IAX2_CODEC_G726:
		return 2;
	case IAX2_CODEC_GSM:
		return 3;
	case IAX2_CODEC_G723_1:
		return 4;
	case IAX2_CODEC_ADPCM:
		return 5;
	case IAX2_CODEC_LPC10:
		return 7;
	case IAX2_CODEC_ALAW:
		return 8;
	case IAX2_CODEC_SLINEAR:
		return 9;
	case IAX2_CODEC_G729A:
		return 18;
	case IAX2_CODEC_ILBC:
		return 97;
	default:
		return -1;
	}

	/* NOT REACHED */
	return -1;
}

static int get_uncompressed_subclass(unsigned char c_sub)
{
        if (c_sub & 0x80) {
		/* 'C' bit is set (refer to standard) */
                if (c_sub == 0xFF)
                        return -1;
                else
                        return 1 << (c_sub & ~0x80 & 0x1F);
        } else {
		/* 'C' bit in SubClass component not set */
                return c_sub;
	}
}

static int parse_iax2_ies(struct iax2_ies *ies, unsigned char *data, int datalen)
{
	int len = 0, ie = 0, odlen = datalen, pass=1;
	CStdString logmsg;

	memset(ies, 0, (int)sizeof(struct iax2_ies));
	while(datalen >= 2) {
		ie = data[0];
		len = data[1];

		//logmsg.Format("Looking up IE %d (len=%d)", ie, len);
                //LOG4CXX_INFO(s_iax2parsersLog, logmsg);

		if (len > datalen - 2) {
			/* Strange.  The quoted length of the IE is past the actual
			 * bounds of the IEs size */
			logmsg.Format("Error parsing IEs Pass=%d Length of IE=%d, "
					"datalen-2=%d, IE=%d, OrigDlen=%d", pass, len, datalen-2, ie, odlen);
			LOG4CXX_INFO(s_iax2parsersLog, logmsg);
			return -1;
		}

		switch(ie) {
		case IAX2_IE_CALLING_NAME:
			ies->calling_name = (char *)data + 2;
			break;
		case IAX2_IE_CALLED_NUMBER:
			ies->callee = (char *)data + 2;
			break;
		case IAX2_IE_CALLING_NUMBER:
			ies->caller = (char *)data + 2;
                        break;
		case IAX2_IE_FORMAT:
			if(len == (int)sizeof(unsigned int))
				ies->format = ntohl(get_unaligned_uint32(data+2));
			else
				ies->format = 0; /* Invalid */
			break;
		case IAX2_IE_USERNAME:
			ies->username = (char *)data + 2;
                        break;
                case IAX2_IE_AUTHMETHODS:
			if(len == (int)sizeof(unsigned int))
                                ies->authmethods = ntohl(get_unaligned_uint32(data+2));
                        else
                                ies->authmethods = 0; /* Invalid */
                        break;
                case IAX2_IE_CHALLENGE:
                        ies->challenge = (char *)data + 2;
                        break;
		default:
			/* Ignore the rest */
			break;
		}

#if 0 /* Debug headaches caused by udpHeader->len */
		char tmpt[256];
		memset(tmpt, 0, sizeof(tmpt));
		memcpy(tmpt, data+2, len);
                logmsg.Format("Got %s", tmpt);
		LOG4CXX_INFO(s_iax2parsersLog, logmsg);
#endif

		data[0] = 0;
		datalen -= (len + 2);
		data += (len + 2);
		pass++;
	}

	*data = '\0';
	if(datalen) {
		/* IE contents likely to be invalid because we should have totally
		 * consumed the entire amount of data */
                CStdString logmsg;

                logmsg.Format("Error parsing IEs. datalen left=%d", len, datalen, ie);
                LOG4CXX_INFO(s_iax2parsersLog, logmsg);

		return -1;
	}

	return 0;
}

void iax2_dump_frame(struct Iax2FullHeader *fh, char *source, char *dest)
{
        const char *frames[] = {
                "(0?)",
                "DTMF   ",
                "VOICE  ",
                "VIDEO  ",
                "CONTROL",
                "NULL   ",
                "IAX    ",
                "TEXT   ",
                "IMAGE  ",
                "HTML   ",
                "CNG    " };
        const char *iaxs[] = {
                "(0?)",
                "NEW    ",
                "PING   ",
                "PONG   ",
                "ACK    ",
                "HANGUP ",
                "REJECT ",
                "ACCEPT ",
                "AUTHREQ",
                "AUTHREP",
                "INVAL  ",
                "LAGRQ  ",
                "LAGRP  ",
                "REGREQ ",
                "REGAUTH",
                "REGACK ",
                "REGREJ ",
                "REGREL ",
                "VNAK   ",
                "DPREQ  ",
                "DPREP  ",
                "DIAL   ",
                "TXREQ  ",
                "TXCNT  ",
                "TXACC  ",
                "TXREADY",
                "TXREL  ",
                "TXREJ  ",
                "QUELCH ",
                "UNQULCH",
                "POKE   ",
                "PAGE   ",
                "MWI    ",
                "UNSPRTD",
                "TRANSFR",
                "PROVISN",
                "FWDWNLD",
                "FWDATA "
        };
        const char *cmds[] = {
                "(0?)",
                "HANGUP ",
                "RING   ",
                "RINGING",
                "ANSWER ",
                "BUSY   ",
                "TKOFFHK",
                "OFFHOOK" };
        char class2[20];
        char subclass2[20];
	CStdString tmp;
        const char *cclass;
        const char *subclass;
        //char *dir;

        if (fh->type >= (int)sizeof(frames)/(int)sizeof(frames[0])) {
                snprintf(class2, sizeof(class2), "(%d?)", fh->type);
                cclass = class2;
        } else {
                cclass = frames[(int)fh->type];
        }

        if (fh->type == IAX2_FRAME_IAX) {
                if (fh->c_sub >= (int)sizeof(iaxs)/(int)sizeof(iaxs[0])) {
                        snprintf(subclass2, sizeof(subclass2), "(%d?)", fh->c_sub);
                        subclass = subclass2;
                } else {
                        subclass = iaxs[(int)fh->c_sub];
                }
        } else if (fh->type == IAX2_FRAME_CONTROL) {
                if (fh->c_sub >= (int)sizeof(cmds)/(int)sizeof(cmds[0])) {
                        snprintf(subclass2, sizeof(subclass2), "(%d?)", fh->c_sub);
                        subclass = subclass2;
                } else {
                        subclass = cmds[(int)fh->c_sub];
                }
        } else {
                snprintf(subclass2, sizeof(subclass2), "%d", fh->c_sub);
                subclass = subclass2;
        }

        tmp.Format("IAX2-Frame -- OSeqno: %3.3d ISeqno: %3.3d Type: %s Subclass: %s",
                 fh->oseqno, fh->iseqno, cclass, subclass);
	LOG4CXX_INFO(s_iax2parsersLog, tmp);
	tmp.Format("   Timestamp: %05lums  SCall: %5.5d  DCall: %5.5d [Source: %s Dest: %s]",
			(unsigned long)ntohl(fh->ts),
			ntohs(fh->scallno) & ~0x8000, ntohs(fh->dcallno) & ~0x8000, source, dest);

	LOG4CXX_INFO(s_iax2parsersLog, tmp);
}

/* See if this is an IAX2 NEW.  If so, process */
bool TryIax2New(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader,
		UdpHeaderStruct* udpHeader, u_char* udpPayload)
{
	struct Iax2FullHeader *fh = (struct Iax2FullHeader *)udpPayload;
	struct iax2_ies ies;
	int ies_len = 0, udp_act_payload_len = 0;
	Iax2NewInfoRef info(new Iax2NewInfo());
	//char source_ip[16], dest_ip[16];
	CStdString logmsg;

	if(!DLLCONFIG.m_iax2Support)
		return false;

	memset(&ies, 0, sizeof(ies));
	udp_act_payload_len = (ntohs(udpHeader->len)-sizeof(UdpHeaderStruct));
	if(udp_act_payload_len < (int)sizeof(*fh))
		return false; /* Frame too small */

        if(!(ntohs(fh->scallno) & 0x8000))
                return false; /* Not a full frame */

	ies_len = ((u_char*)ipHeader+ntohs(ipHeader->ip_len))-(udpPayload+sizeof(*fh));

#if 0  /* Debug headaches caused by udpHeader->len */
	/* Beware that udpHeader->len is not the length of the udpPayload
	 * but rather this includes the length of the UDP header as well.
	 * I.e watch out for the figure "8" as you debug ;-) */
	{
		char source_ip[16], dest_ip[16];

		ACE_OS::inet_ntop(AF_INET, (void*)&ipHeader->ip_src, source_ip, sizeof(source_ip));
		ACE_OS::inet_ntop(AF_INET, (void*)&ipHeader->ip_dest, dest_ip, sizeof(dest_ip));
	        iax2_dump_frame(fh, source_ip, dest_ip);
	}

        logmsg.Format("UDP_Payload=%p UDP+FH_Payload=%p FH->IEDATA=%p ies_len=%d "
                        "udpHeader->len-sizeof(fullhdr)=%d (ntohs(udpHeader->len)"
                        "-sizeof(UdpHeaderStruct))=%d", udpPayload, udpPayload+
                        sizeof(*fh), fh->ie_data, ies_len, (ntohs(udpHeader->len)-
                        sizeof(*fh)), (ntohs(udpHeader->len)-sizeof(UdpHeaderStruct)));
        LOG4CXX_INFO(s_iax2parsersLog, logmsg);
#endif

	if(fh->type != IAX2_FRAME_IAX)
		return false; /* Frame type must be IAX */

	if(get_uncompressed_subclass(fh->c_sub) != IAX2_COMMAND_NEW)
		return false; /* Subclass must be NEW */

	if(parse_iax2_ies(&ies, fh->ie_data, ies_len))
		return false; /* Invalid "full" frame received */

	if(!ies.callee)
		return false; /* According to the SPEC, a NEW MUST have a
			       * callee (Called Number) */

	if(!strlen(ies.callee))
		return false; /* According to the SPEC, a NEW MUST have a
                               * callee (Called Number) */

	if(!ies.caller) {
		ies.caller = (char*)"WITHELD";
	} else {
		if(!strlen(ies.caller)) {
			ies.caller = (char*)"WITHELD";
		}
	}

	/* Statistically this is most likely a NEW IAX2 frame. */

	info->m_senderIp = ipHeader->ip_src;
	info->m_receiverIp = ipHeader->ip_dest;
	info->m_caller = CStdString(ies.caller);
	info->m_callee = CStdString(ies.callee);
	info->m_callingName = CStdString(ies.calling_name ? ies.calling_name : "");
	info->m_callNo = IntToString(ntohs(fh->scallno) & ~0x8000);

	/* Report the packet */
	Iax2SessionsSingleton::instance()->ReportIax2New(info);

	LOG4CXX_INFO(s_iax2parsersLog, "Processed IAX2 NEW frame ts:" + IntToString(ntohl(fh->ts)));

	return true;
}

bool TryIax2Accept(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader,
                UdpHeaderStruct* udpHeader, u_char* udpPayload)
{
        struct Iax2FullHeader *fh = (struct Iax2FullHeader *)udpPayload;
        struct iax2_ies ies;
        int ies_len = 0, udp_act_payload_len = 0;
        Iax2AcceptInfoRef info(new Iax2AcceptInfo());

        if(!DLLCONFIG.m_iax2Support)
                return false;

        memset(&ies, 0, sizeof(ies));
        udp_act_payload_len = (ntohs(udpHeader->len)-sizeof(UdpHeaderStruct));
        if(udp_act_payload_len < (int)sizeof(*fh))
                return false; /* Frame too small */

        if(!(ntohs(fh->scallno) & 0x8000))
                return false; /* Not a full frame */

        if(fh->type != IAX2_FRAME_IAX)
                return false; /* Frame type must be IAX */

        if(get_uncompressed_subclass(fh->c_sub) != IAX2_COMMAND_ACCEPT)
                return false; /* Subclass must be ACCEPT */

        ies_len = ((u_char*)ipHeader+ntohs(ipHeader->ip_len))-(udpPayload+sizeof(*fh));

	/* In this case, this just serves to test the integrity of the
	 * Information Elements */
        if(parse_iax2_ies(&ies, fh->ie_data, ies_len))
                return false; /* Invalid "full" frame received */

	if(!ies.format)
		return false; /* According to the SPEC, ACCEPT must have
			       * a format specified */

	/* We have an ACCEPT */

        info->m_senderIp = ipHeader->ip_src;
        info->m_receiverIp = ipHeader->ip_dest;
	info->m_sender_callno = IntToString(ntohs(fh->scallno) & ~0x8000);
	info->m_receiver_callno = IntToString(ntohs(fh->dcallno) & ~0x8000);

        Iax2SessionsSingleton::instance()->ReportIax2Accept(info);

	LOG4CXX_INFO(s_iax2parsersLog, "Processed IAX2 ACCEPT frame ts:" + IntToString(ntohl(fh->ts)));

        return true;
}

bool TryIax2Authreq(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader,
                UdpHeaderStruct* udpHeader, u_char* udpPayload)
{
        struct Iax2FullHeader *fh = (struct Iax2FullHeader *)udpPayload;
        struct iax2_ies ies;
        int ies_len = 0, udp_act_payload_len = 0;
	Iax2AuthreqInfoRef info(new Iax2AuthreqInfo());

        if(!DLLCONFIG.m_iax2Support)
                return false;

        memset(&ies, 0, sizeof(ies));
        udp_act_payload_len = (ntohs(udpHeader->len)-sizeof(UdpHeaderStruct));
        if(udp_act_payload_len < (int)sizeof(*fh))
                return false; /* Frame too small */

	if(!(ntohs(fh->scallno) & 0x8000))
                return false; /* Not a full frame */

        if(fh->type != IAX2_FRAME_IAX)
                return false; /* Frame type must be IAX */

        if(get_uncompressed_subclass(fh->c_sub) != IAX2_COMMAND_AUTHREQ)
                return false; /* Subclass must be AUTHREQ */

	ies_len = ((u_char*)ipHeader+ntohs(ipHeader->ip_len))-(udpPayload+sizeof(*fh));

        /* In this case, this just serves to test the integrity of the
         * Information Elements */
        if(parse_iax2_ies(&ies, fh->ie_data, ies_len))
                return false; /* Invalid "full" frame received */


	if(!ies.username)
		return false;  /* According to the spec AUTHREQ must have
			        * a user name.  Can it be empty? */

	/*
	if(!strlen(ies.username))
		return false;  * According to the spec AUTHREQ must have
                               * a user name. *
	*/

	if(!ies.authmethods)
		return false;  /* According to the spec AUTHREQ must have
                                * AUTHMETHODS */

	if(!ies.challenge)
                return false;  /* According to the spec, AUTHREQ must have
			        * a CHALLENGE string.  Can it be empty? */


	/* We have an AUTHREQ */
        info->m_senderIp = ipHeader->ip_src;
        info->m_receiverIp = ipHeader->ip_dest;
        info->m_sender_callno = IntToString(ntohs(fh->scallno) & ~0x8000);
        info->m_receiver_callno = IntToString(ntohs(fh->dcallno) & ~0x8000);

        /* Report the packet */
        Iax2SessionsSingleton::instance()->ReportIax2Authreq(info);

	LOG4CXX_INFO(s_iax2parsersLog, "Processed IAX2 AUTHREQ frame");

        return true;
}

/* HANGUP via IAX frame */
bool TryIax2Hangup(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader,
                UdpHeaderStruct* udpHeader, u_char* udpPayload)
{
        struct Iax2FullHeader *fh = (struct Iax2FullHeader *)udpPayload;
        struct iax2_ies ies;
        int ies_len = 0, udp_act_payload_len = 0;
	Iax2HangupInfoRef info(new Iax2HangupInfo());

        if(!DLLCONFIG.m_iax2Support)
                return false;

        memset(&ies, 0, sizeof(ies));
        udp_act_payload_len = (ntohs(udpHeader->len)-sizeof(UdpHeaderStruct));
        if(udp_act_payload_len < (int)sizeof(*fh))
                return false; /* Frame too small */

	if(!(ntohs(fh->scallno) & 0x8000))
                return false; /* Not a full frame */

        if(fh->type != IAX2_FRAME_IAX)
                return false; /* Frame type must be IAX */

        if(get_uncompressed_subclass(fh->c_sub) != IAX2_COMMAND_HANGUP)
                return false; /* Subclass must be HANGUP */

        ies_len = ((u_char*)ipHeader+ntohs(ipHeader->ip_len))-(udpPayload+sizeof(*fh));

        /* In this case, this just serves to test the integrity of the
         * Information Elements */
        if(parse_iax2_ies(&ies, fh->ie_data, ies_len))
                return false; /* Invalid "full" frame received */

	/* We have a HANGUP */

        info->m_senderIp = ipHeader->ip_src;
        info->m_receiverIp = ipHeader->ip_dest;
        info->m_sender_callno = IntToString(ntohs(fh->scallno) & ~0x8000);
        info->m_receiver_callno = IntToString(ntohs(fh->dcallno) & ~0x8000);

        /* Report the packet */
        Iax2SessionsSingleton::instance()->ReportIax2Hangup(info);

	CStdString logMsg;
	info->ToString(logMsg);
        LOG4CXX_INFO(s_iax2parsersLog, "Processed IAX2 HANGUP frame: " + logMsg);

	return true;
}

/* HANGUP via CONTROL frame */
bool TryIax2ControlHangup(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader,
                UdpHeaderStruct* udpHeader, u_char* udpPayload)
{
        struct Iax2FullHeader *fh = (struct Iax2FullHeader *)udpPayload;
        Iax2HangupInfoRef info(new Iax2HangupInfo());
	int udp_act_payload_len = 0;

        if(!DLLCONFIG.m_iax2Support)
                return false;

        udp_act_payload_len = (ntohs(udpHeader->len)-sizeof(UdpHeaderStruct));
        if(udp_act_payload_len < (int)sizeof(*fh))
                return false; /* Frame too small */

	if(!(ntohs(fh->scallno) & 0x8000))
                return false; /* Not a full frame */

        if(fh->type != IAX2_FRAME_CONTROL)
                return false; /* Frame type must be CONTROL */

        if(get_uncompressed_subclass(fh->c_sub) != IAX2_CONTROL_HANGUP)
                return false; /* Subclass must be HANGUP */

        /* We have a HANGUP */

        info->m_senderIp = ipHeader->ip_src;
        info->m_receiverIp = ipHeader->ip_dest;
        info->m_sender_callno = IntToString(ntohs(fh->scallno) & ~0x8000);
        info->m_receiver_callno = IntToString(ntohs(fh->dcallno) & ~0x8000);

        /* Report the packet */
        Iax2SessionsSingleton::instance()->ReportIax2Hangup(info);

        LOG4CXX_INFO(s_iax2parsersLog, "Processed IAX2 CONTROL HANGUP frame");

        return true;
}

bool TryIax2Reject(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader,
                UdpHeaderStruct* udpHeader, u_char* udpPayload)
{
        struct Iax2FullHeader *fh = (struct Iax2FullHeader *)udpPayload;
        struct iax2_ies ies;
        int ies_len = 0, udp_act_payload_len = 0;
	Iax2HangupInfoRef info(new Iax2HangupInfo());

        if(!DLLCONFIG.m_iax2Support)
                return false;

        memset(&ies, 0, sizeof(ies));
        udp_act_payload_len = (ntohs(udpHeader->len)-sizeof(UdpHeaderStruct));
        if(udp_act_payload_len < (int)sizeof(*fh))
                return false; /* Frame too small */

	if(!(ntohs(fh->scallno) & 0x8000))
                return false; /* Not a full frame */

        if(fh->type != IAX2_FRAME_IAX)
                return false; /* Frame type must be IAX */

        if(get_uncompressed_subclass(fh->c_sub) != IAX2_COMMAND_REJECT)
                return false; /* Subclass must be REJECT */

	ies_len = ((u_char*)ipHeader+ntohs(ipHeader->ip_len))-(udpPayload+sizeof(*fh));

        /* In this case, this just serves to test the integrity of the
         * Information Elements */
        if(parse_iax2_ies(&ies, fh->ie_data, ies_len))
                return false; /* Invalid "full" frame received */

        /* We have a REJECT */

        info->m_senderIp = ipHeader->ip_src;
        info->m_receiverIp = ipHeader->ip_dest;
        info->m_sender_callno = IntToString(ntohs(fh->scallno) & ~0x8000);
        info->m_receiver_callno = IntToString(ntohs(fh->dcallno) & ~0x8000);

        /* Report the packet */
        Iax2SessionsSingleton::instance()->ReportIax2Hangup(info);

        LOG4CXX_INFO(s_iax2parsersLog, "Processed IAX2 REJECT frame");

        return true;
}

bool TryIax2FullVoiceFrame(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader,
                UdpHeaderStruct* udpHeader, u_char* udpPayload)
{
        struct Iax2FullHeader *fh = (struct Iax2FullHeader *)udpPayload;
        int data_len = 0, codec = 0, pt = 0, udp_act_payload_len = 0;
	Iax2PacketInfoRef info(new Iax2PacketInfo());

        if(!DLLCONFIG.m_iax2Support)
                return false;

        udp_act_payload_len = (ntohs(udpHeader->len)-sizeof(UdpHeaderStruct));
        if(udp_act_payload_len < (int)sizeof(*fh))
                return false; /* Frame too small */

        if(!(ntohs(fh->scallno) & 0x8000))
                return false; /* Not a full frame */

        if(fh->type != IAX2_FRAME_VOICE)
                return false; /* Frame type must be VOICE */

	codec = get_uncompressed_subclass(fh->c_sub);
	if((pt = iax2_codec_to_rtp_payloadtype(codec)) < 0) {
		CStdString logmsg;

		logmsg.Format("Invalid payload type %d received for "
				"IAX_FRAME_VOICE, IAX2 codec %d", pt, codec);
		LOG4CXX_INFO(s_iax2parsersLog, logmsg);
		return false; /* Invalid codec type received */
	}

	data_len = ((u_char*)ipHeader+ntohs(ipHeader->ip_len))-(udpPayload+sizeof(*fh));
	if(data_len == 0)
		return false; /* Empty packet? */

        /* We have a full VOICE frame */

	info->m_sourceIp = ipHeader->ip_src;
	info->m_destIp = ipHeader->ip_dest;
	info->m_sourcecallno = (ntohs(fh->scallno) & ~0x8000);
	info->m_destcallno = (ntohs(fh->dcallno) & ~0x8000);
	info->m_payloadSize = data_len;
	info->m_payload = udpPayload+sizeof(*fh);
	info->m_payloadType = pt;
	info->m_timestamp = ntohl(fh->ts);
	info->m_arrivalTimestamp = time(NULL);
	info->m_frame_type = IAX2_FRAME_FULL;

	Iax2SessionsSingleton::instance()->ReportIax2Packet(info);

	CStdString logmsg, packetInfo;
	info->ToString(packetInfo);
	logmsg.Format("Processed IAX2 FULL VOICE frame pt:%d", pt);
        LOG4CXX_INFO(s_iax2parsersLog, logmsg);
	if(s_iax2parsersLog->isDebugEnabled())
	{
		LOG4CXX_DEBUG(s_iax2parsersLog, packetInfo);
	}

	return true;
}

bool TryIax2MetaTrunkFrame(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader,
                UdpHeaderStruct* udpHeader, u_char* udpPayload)
{
	struct Iax2MetaTrunkHeader *mh = (struct Iax2MetaTrunkHeader *)udpPayload;
	struct Iax2MetaTrunkEntry *supermini = NULL;
	struct Iax2MetaTrunkEntryTs *mini = NULL;
	int content_type = 0; /* 0 means mini frames, 1 means super mini (no timestampes) */
	unsigned int data_len = 0;
	int entries = 0, udp_act_payload_len = 0;
	Iax2PacketInfoRef info(new Iax2PacketInfo());

        if(!DLLCONFIG.m_iax2Support)
                return false;

        udp_act_payload_len = (ntohs(udpHeader->len)-sizeof(UdpHeaderStruct));
        if(udp_act_payload_len < (int)sizeof(*mh))
                return false; /* Frame too small */

	if(mh->meta != 0)
		return false; /* Must be zero */

	if(mh->metacmd & 0x80)
		return false; /* 'V' bit must be set to zero */

	if(mh->metacmd != 1)
		return false; /* metacmd must be 1 */

	/* Get the length of the information apart from the
	 * meta trunk header */
	data_len = ((u_char*)ipHeader+ntohs(ipHeader->ip_len))-(udpPayload+sizeof(*mh));
        if(data_len == 0)
                return false; /* Empty packet? */

	/* Step over the meta trunk header */
	udpPayload += sizeof(*mh);

	/* Determine whether the trunk contents have their own
	 * timestamps or not */
	if(mh->cmddata & 0x01)
		content_type = 1;
	else
		content_type = 0;

	if(content_type) {
		/* Have timestamps */

		while(data_len) {
			if(data_len < sizeof(*mini))
				break;

			mini = (struct Iax2MetaTrunkEntryTs *)udpPayload;

			if(data_len < sizeof(*mini)+ntohs(mini->len))
				break;

			info->m_sourceIp = ipHeader->ip_src;
			info->m_destIp = ipHeader->ip_dest;
			info->m_sourcecallno = (ntohs(mini->mini.scallno) & ~0x8000);
			info->m_destcallno = 0;
			info->m_payloadSize = ntohs(mini->len);
			info->m_payload = udpPayload+sizeof(*mini);
			info->m_payloadType = 0;
			info->m_timestamp = ntohs(mini->mini.ts);
			info->m_arrivalTimestamp = time(NULL);
			info->m_frame_type = IAX2_FRAME_MINI;

			Iax2SessionsSingleton::instance()->ReportIax2Packet(info);
			entries += 1;

			udpPayload += sizeof(*mini)+ntohs(mini->len);
			data_len -= sizeof(*mini)+ntohs(mini->len);
		}
	} else {
		/* Have no timestamps */
		while(data_len) {
			if(data_len < sizeof(*supermini))
				break;

			supermini = (struct Iax2MetaTrunkEntry *)udpPayload;

                        if(data_len < sizeof(*supermini)+ntohs(supermini->len))
				break;

			info->m_sourceIp = ipHeader->ip_src;
                        info->m_destIp = ipHeader->ip_dest;
                        info->m_sourcecallno = (ntohs(supermini->scallno) & ~0x8000);
                        info->m_destcallno = 0;
                        info->m_payloadSize = ntohs(supermini->len);
                        info->m_payload = udpPayload+sizeof(*supermini);
                        info->m_payloadType = 0;
                        info->m_timestamp = 0;
                        info->m_arrivalTimestamp = time(NULL);
                        info->m_frame_type = IAX2_FRAME_MINI;

                        Iax2SessionsSingleton::instance()->ReportIax2Packet(info);
			entries += 1;

                        udpPayload += sizeof(*supermini)+ntohs(supermini->len);
                        data_len -= sizeof(*supermini)+ntohs(supermini->len);
		}
	}


	if(entries > 0) {
		CStdString logmsg;

		logmsg.Format("Processed IAX2 Meta Trunk packet with %d entries", entries);
	        LOG4CXX_DEBUG(s_iax2parsersLog, logmsg);
		return true;
	}

	return false; /* No valid entries in this so-called meta trunk frame */
}

bool TryIax2MiniVoiceFrame(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader,
                UdpHeaderStruct* udpHeader, u_char* udpPayload)
{
	struct Iax2MiniHeader *mini = (struct Iax2MiniHeader *)udpPayload;
        int data_len = 0, udp_act_payload_len = 0;
	Iax2PacketInfoRef info(new Iax2PacketInfo());

        if(!DLLCONFIG.m_iax2Support)
                return false;

        udp_act_payload_len = (ntohs(udpHeader->len)-sizeof(UdpHeaderStruct));
        if(udp_act_payload_len < (int)sizeof(*mini))
                return false; /* Frame too small */

        if((ntohs(mini->scallno) & 0x8000))
                return false; /* Not a Mini frame */

	data_len = ((u_char*)ipHeader+ntohs(ipHeader->ip_len))-(udpPayload+sizeof(*mini));
        if(data_len == 0)
                return false; /* Empty packet? */

        info->m_sourceIp = ipHeader->ip_src;
        info->m_destIp = ipHeader->ip_dest;
        info->m_sourcecallno = (ntohs(mini->scallno) & ~0x8000);
        info->m_destcallno = 0;
        info->m_payloadSize = data_len;
        info->m_payload = udpPayload+sizeof(*mini);
        info->m_payloadType = 0;
	info->m_timestamp = ntohs(mini->ts);
        info->m_arrivalTimestamp = time(NULL);
        info->m_frame_type = IAX2_FRAME_MINI;

	CStdString logMsg, packetInfo;
	info->ToString(packetInfo);
	logMsg.Format("IAX2 mini frame: %s", packetInfo);
	LOG4CXX_DEBUG(s_iax2parsersLog, logMsg);

        return Iax2SessionsSingleton::instance()->ReportIax2Packet(info);
}
