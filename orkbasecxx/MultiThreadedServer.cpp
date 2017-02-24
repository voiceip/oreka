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

#include "ace/INET_Addr.h"
#include "ace/OS_NS_string.h"
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/dom/DOMWriter.hpp>
#include <xercesc/dom/DOMImplementation.hpp>
#include <xercesc/dom/DOMImplementationRegistry.hpp>

#include "ObjectFactory.h"
#include "serializers/SingleLineSerializer.h"
#include "serializers/DomSerializer.h"
#include "serializers/UrlSerializer.h"
#include "Utils.h"
#include "MultiThreadedServer.h"
#include "EventStreaming.h"

#if !defined(MSG_NOSIGNAL)
#define MSG_NOSIGNAL 0
#endif

log4cxx::LoggerPtr CommandLineServer::s_log;

// This is run at the start of each connection
int CommandLineServer::open (void *void_acceptor)
{
	LOG4CXX_INFO(s_log, "new connection");
	return this->activate (THR_DETACHED);
}

// This is run at program initialization
void CommandLineServer::run(void* args)
{
	s_log = log4cxx::Logger::getLogger("interface.commandlineserver");

	unsigned short tcpPort = (unsigned short)(unsigned long)args;
	//unsigned short tcpPort = (unsigned short)*(int*)args;

	CommandLineAcceptor peer_acceptor;
	ACE_INET_Addr addr (tcpPort);
	ACE_Reactor reactor;
	CStdString tcpPortString = IntToString(tcpPort);

	if (peer_acceptor.open (addr, &reactor) == -1)
	{
		LOG4CXX_ERROR(s_log, CStdString("Failed to start command line server on port:") + tcpPortString + CStdString(" do you have another instance of orkaudio running?"));
	}
	else
	{
		LOG4CXX_INFO(s_log, CStdString("Started command line server on port:")+tcpPortString);
		for(;;)
		{
			reactor.handle_events();
		}
	}
}

int CommandLineServer::svc(void)
{
	for (bool active = true;active == true;)
	{
		char buf[2048];
		ACE_Time_Value timeout;
		timeout.sec(3600);
		int i = 0;

		// Display prompt
		char prompt[] = "\r\n>";
		peer().send(prompt, 3, MSG_NOSIGNAL);

		// Get one command line
		bool foundCRLF = false;
		while(active && !foundCRLF && i<2040)
		{
			ssize_t size = peer().recv(buf+i, 2040-i, &timeout);

			if (size == 0 || size == -1)
			{
				active = false;
			}
			else
			{
				for(int j=0; j<size && !foundCRLF;j++)
				{
                                        if (buf[i+j] < 32 || buf[i+j] > 126)
                                        {
                                                // detected a char that cannot be part of an URL
                                                LOG4CXX_WARN(s_log, "detected command URL with invalid character(s), ignoring");
                                                return 0;
                                        }
					else if(buf[i+j] == '\r' || buf[i+j] == '\n')
					{
						foundCRLF = true;
						buf[i+j] = '\0';
						CStdString command(buf);
						try
						{
							CStdString className = SingleLineSerializer::FindClass(command);
							ObjectRef objRef = ObjectFactory::GetSingleton()->NewInstance(className);
							if (objRef.get())
							{
								objRef->DeSerializeSingleLine(command);
								ObjectRef response = objRef->Process();
								CStdString responseString = response->SerializeSingleLine();
								peer().send((PCSTR)responseString, responseString.GetLength(), MSG_NOSIGNAL);
							}
							else
							{
								CStdString error = "Unrecognized command";
								peer().send(error, error.GetLength(), MSG_NOSIGNAL);							;
							}
						}
						catch (CStdString& e)
						{
							peer().send(e, e.GetLength(), MSG_NOSIGNAL);							;
						}
					}
				}
				i += size;
			}
		}
	}
	return 0;
}


//==============================================================

log4cxx::LoggerPtr HttpServer::s_log;

// This is run at the start of each connection
int HttpServer::open (void *void_acceptor)
{
	return this->activate (THR_DETACHED);
}

// This is run at program initialization
void HttpServer::run(void* args)
{
	s_log = log4cxx::Logger::getLogger("interface.httpserver");

	unsigned short tcpPort = (unsigned short)(unsigned long)args;
	//unsigned short tcpPort = (unsigned short)*(int*)args;

	HttpAcceptor peer_acceptor;
	ACE_INET_Addr addr (tcpPort);
	ACE_Reactor reactor;
	CStdString tcpPortString = IntToString(tcpPort);

	if (peer_acceptor.open (addr, &reactor) == -1)
	{
		LOG4CXX_ERROR(s_log, CStdString("Failed to start http server on port:") + tcpPortString  + CStdString(" do you have another instance of orkaudio running?"));
	}
	else
	{
		LOG4CXX_INFO(s_log, CStdString("Started HTTP server on port:")+tcpPortString);
		for(;;)
		{
			reactor.handle_events();
		}
	}
}

int HttpServer::svc(void)
{
	char buf[2048];
	buf[2047] = '\0';	// security
	ACE_Time_Value timeout;
	timeout.sec(5);

	ssize_t size = peer().recv(buf, 2040, &timeout);

	if (size > 5)
	{
		try
		{
			int startUrlOffset = 5;		// get rid of "GET /" from Http request, so skip 5 chars
			char* stopUrl = ACE_OS::strstr(buf+startUrlOffset, " HTTP");	// detect location of post-URL trailing stuff
			if(!stopUrl)
			{
                                LOG4CXX_WARN(s_log, "Malformed http request");
				throw (CStdString("Malformed http request"));							;
			}
			*stopUrl = '\0';									// Remove post-URL trailing stuff
                        int urlLen = stopUrl - buf;
                        for(int i = startUrlOffset; i< urlLen; i++)
                        {
                                if(buf[i] < 33 || buf[i] > 126)
                                {
                                        // detected a char that cannot be part of an URL
                                        LOG4CXX_WARN(s_log, "detected command URL with invalid character(s), ignoring");
                                        throw (CStdString("Malformed command URL with invalid character(s)"));		
                                }
                        }

			CStdString url(buf+startUrlOffset);
			int queryOffset = url.Find("?");
			if (queryOffset	 > 0)
			{
				// Strip beginning of URL in case the command is received as an URL "query" of the form:
				// http://hostname/service/command?type=ping
				url = url.Right(url.size() - queryOffset - 1);
			}


			CStdString className = UrlSerializer::FindClass(url);
			ObjectRef objRef = ObjectFactory::GetSingleton()->NewInstance(className);
			if (objRef.get())
			{
                                LOG4CXX_INFO(s_log, "command: " + url);
				objRef->DeSerializeUrl(url);
				ObjectRef response = objRef->Process();

				if(response.get() == NULL)
				{
                                        LOG4CXX_WARN(s_log, "Command does not return a response:" + className);
					throw (CStdString("Command does not return a response:") + className);
				}
				else
				{
					DOMImplementation* impl =  DOMImplementationRegistry::getDOMImplementation(XStr("Core").unicodeForm());
					XERCES_CPP_NAMESPACE::DOMDocument* myDoc;
					   myDoc = impl->createDocument(
								   0,			 // root element namespace URI.
								   XStr("response").unicodeForm(),	   // root element name
								   0);			 // document type object (DTD).
					response->SerializeDom(myDoc);
					CStdString pingResponse = DomSerializer::DomNodeToString(myDoc);

					CStdString httpOk("HTTP/1.0 200 OK\r\nContent-type: text/xml\r\n\r\n");
                                        CStdString singleLineResponseString = response->SerializeSingleLine();
                                        LOG4CXX_INFO(s_log, "response: " + singleLineResponseString);
					peer().send(httpOk, httpOk.GetLength(), MSG_NOSIGNAL);
					peer().send(pingResponse, pingResponse.GetLength(), MSG_NOSIGNAL);

					myDoc->release();
				}
			}
			else
			{
                                LOG4CXX_WARN(s_log, "Command not found:" + className);
				throw (CStdString("Command not found:") + className);							;
			}

		}
		catch (CStdString &e)
		{
			CStdString error("HTTP/1.0 404 not found\r\nContent-type: text/html\r\n\r\nError\r\n");
			error = error + e + "\r\n";
			peer().send(error, error.GetLength(), MSG_NOSIGNAL);
		}
		catch(const XMLException& e)
		{
			CStdString error("HTTP/1.0 404 not found\r\nContent-type: text/html\r\n\r\nXML Error\r\n");
			peer().send(error, error.GetLength(), MSG_NOSIGNAL);
		}
	}
	else
	{
		CStdString notFound("HTTP/1.0 404 not found\r\nContent-type: text/html\r\n\r\nNot found\r\n");
		peer().send(notFound, notFound.GetLength(), MSG_NOSIGNAL);
	}
	return 0;
}

//==============================================

log4cxx::LoggerPtr EventStreamingServer::s_log;

int EventStreamingServer::open(void *void_acceptor)
{
	return this->activate (THR_DETACHED);
}

void EventStreamingServer::run(void* args)
{
	unsigned short tcpPort = (unsigned short)(ACE_UINT64)args;
	CStdString tcpPortString = IntToString(tcpPort);
	EventStreamingAcceptor peer_acceptor;
	ACE_INET_Addr addr (tcpPort);
	ACE_Reactor reactor;

	s_log = log4cxx::Logger::getLogger("interface.eventstreamingserver");

	if (peer_acceptor.open (addr, &reactor) == -1)
	{
		LOG4CXX_ERROR(s_log, CStdString("Failed to start event streaming server on port:") + tcpPortString + CStdString(" do you have another instance of orkaudio running?"));
	}
	else
	{
		LOG4CXX_INFO(s_log, CStdString("Started event streaming server on port:")+tcpPortString);
		for(;;)
		{
			reactor.handle_events();
		}
	}
}

int EventStreamingServer::svc(void)
{
	ACE_Time_Value timeout;
	char buf[2048];
	CStdString logMsg;
	CStdString sessionId;
	int messagesSent = 0;

	ssize_t size = peer().recv(buf, 2040);

	if(size <= 5)
	{
		CStdString notFound("HTTP/1.0 404 not found\r\nContent-type: text/html\r\n\r\nNot found\r\n");
		peer().send(notFound, notFound.GetLength(), MSG_NOSIGNAL);
		return 0;
	}

	try
	{
		int startUrlOffset = 5;
		char* stopUrl = ACE_OS::strstr(buf+startUrlOffset, " HTTP");

		if(!stopUrl)
		{
			throw (CStdString("Malformed http request"));
		}

		CStdString header;
		struct tm date = {0};
		time_t now = time(NULL);
		CStdString rfc822Date;

		ACE_OS::gmtime_r(&now, &date);
		rfc822Date.Format("Tue, %.2d Nov %.4d %.2d:%.2d:%.2d GMT", date.tm_mday, (date.tm_year+1900), date.tm_hour, date.tm_min, date.tm_sec);
		header.Format("HTTP/1.1 200 OK\r\nLast-Modified:%s\r\nContent-Type:text/plain\r\n\r\n", rfc822Date);
		peer().send(header, header.GetLength(), MSG_NOSIGNAL);

		time_t startTime = time(NULL);

		sessionId = EventStreamingSingleton::instance()->GetNewSessionId() + " -";
		logMsg.Format("%s Event streaming start", sessionId);
		LOG4CXX_INFO(s_log, logMsg);

		EventStreamingSessionRef session(new EventStreamingSession());
		EventStreamingSingleton::instance()->AddSession(session);

		int sendRes = 0;
		while(sendRes >= 0)
		{
			session->WaitForMessages();

			while(session->GetNumMessages() && sendRes >= 0)
			{
				MessageRef message;

				session->GetTapeMessage(message);
				if(message.get())
				{
					CStdString msgAsSingleLineString;
					msgAsSingleLineString = message->SerializeUrl() + "\r\n";

					sendRes = peer().send(msgAsSingleLineString, msgAsSingleLineString.GetLength(), MSG_NOSIGNAL);
					if(sendRes >= 0)
					{
						messagesSent += 1;
					}
				}
			}
		}

		EventStreamingSingleton::instance()->RemoveSession(session);
		logMsg.Format("%s Stream client stop - sent %d messages in %d sec", sessionId, messagesSent, (time(NULL) - startTime));
		LOG4CXX_INFO(s_log, logMsg);
	}
	catch (CStdString& e)
	{
		CStdString error("HTTP/1.0 404 not found\r\nContent-type: text/html\r\n\r\nError\r\n");
		error = error + e + "\r\n";
		LOG4CXX_ERROR(s_log, e);
		peer().send(error, error.GetLength(), MSG_NOSIGNAL);
	}

	return 0;
}
