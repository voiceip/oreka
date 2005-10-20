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

#include "MultiThreadedServer.h"

#include "ace/INET_Addr.h"
#include "ace/OS_NS_string.h"
#include "ObjectFactory.h"
#include "serializers/SingleLineSerializer.h"
#include "serializers/DomSerializer.h"
#include "serializers/UrlSerializer.h"
#include "LogManager.h"
#include "Utils.h"
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/dom/DOMWriter.hpp>
#include <xercesc/dom/DOMImplementation.hpp>
#include <xercesc/dom/DOMImplementationRegistry.hpp>


int CommandLineServer::open (void *void_acceptor)
{
	return this->activate (THR_DETACHED);
}


void CommandLineServer::run(void* args)
{
	unsigned short tcpPort = (unsigned short)args;
	CommandLineAcceptor peer_acceptor;
	ACE_INET_Addr addr (tcpPort);
	ACE_Reactor reactor;

	if (peer_acceptor.open (addr, &reactor) == -1)
	{
		CStdString tcpPortString = IntToString(tcpPort);
		LOG4CXX_ERROR(LOG.rootLog, CStdString("Failed to start command line server on port:") + tcpPortString);
	}
	else
	{
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
		peer().send(prompt, 3);

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
					if(buf[i+j] == '\r' || buf[i+j] == '\n')
					{
						foundCRLF = true;
						buf[i+j] = '\0';
						CStdString command(buf);
						try
						{
							CStdString className = SingleLineSerializer::FindClass(command);
							ObjectRef objRef = ObjectFactorySingleton::instance()->NewInstance(className);
							if (objRef.get())
							{
								objRef->DeSerializeSingleLine(command);
								ObjectRef response = objRef->Process();
								CStdString responseString = response->SerializeSingleLine();
								peer().send((PCSTR)responseString, responseString.GetLength());
							}
							else
							{
								CStdString error = "Unrecognized command";
								peer().send(error, error.GetLength());							;
							}
						}
						catch (CStdString& e)
						{
							peer().send(e, e.GetLength());							;
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

int HttpServer::open (void *void_acceptor)
{
	return this->activate (THR_DETACHED);
}


void HttpServer::run(void* args)
{
	unsigned short tcpPort = (unsigned short)args;
	HttpAcceptor peer_acceptor;
	ACE_INET_Addr addr (tcpPort);
	ACE_Reactor reactor;

	if (peer_acceptor.open (addr, &reactor) == -1)
	{
		CStdString tcpPortString = IntToString(tcpPort);
		LOG4CXX_ERROR(LOG.rootLog, CStdString("Failed to start http server on port:") + tcpPortString);
	}
	else
	{
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

	ssize_t size = peer().recv(buf, 2040);

	if (size > 5)
	{
		try
		{
			int startUrlOffset = 5;		// get rid of "GET /" from Http request, so skip 5 chars
			char* stopUrl = ACE_OS::strstr(buf+startUrlOffset, " HTTP");	// detect location of post-URL trailing stuff
			if(!stopUrl)
			{
				throw (CStdString("Malformed http request"));							;
			}
			*stopUrl = '\0';									// Remove post-URL trailing stuff	
			CStdString url(buf+startUrlOffset);
			int queryOffset = url.Find("?");
			if (queryOffset  > 0)
			{
				// Strip beginning of URL in case the command is received as an URL "query" of the form:
				// http://hostname/service/command?type=ping
				url = url.Right(url.size() - queryOffset - 1);
			}
		

			CStdString className = UrlSerializer::FindClass(url);
			ObjectRef objRef = ObjectFactorySingleton::instance()->NewInstance(className);
			if (objRef.get())
			{
				objRef->DeSerializeUrl(url);
				ObjectRef response = objRef->Process();

				DOMImplementation* impl =  DOMImplementationRegistry::getDOMImplementation(XStr("Core").unicodeForm());
				XERCES_CPP_NAMESPACE::DOMDocument* myDoc;
				   myDoc = impl->createDocument(
							   0,                    // root element namespace URI.
							   XStr("response").unicodeForm(),         // root element name
							   0);                   // document type object (DTD).
				response->SerializeDom(myDoc);
				CStdString pingResponse = DomSerializer::DomNodeToString(myDoc);

				CStdString httpOk("HTTP/1.0 200 OK\r\nContent-type: text/xml\r\n\r\n");
				peer().send(httpOk, httpOk.GetLength());
				peer().send(pingResponse, pingResponse.GetLength());
			}
			else
			{
				throw (CStdString("Command not found:") + className);							;
			}

		}
		catch (CStdString &e)
		{
			CStdString error("HTTP/1.0 404 not found\r\nContent-type: text/html\r\n\r\nError\r\n");
			error = error + e + "\r\n";
			peer().send(error, error.GetLength());
		}
		catch(const XMLException& e)
		{
			CStdString error("HTTP/1.0 404 not found\r\nContent-type: text/html\r\n\r\nXML Error\r\n");
			peer().send(error, error.GetLength());
		}
	}
	else
	{
		CStdString notFound("HTTP/1.0 404 not found\r\nContent-type: text/html\r\n\r\nNot found\r\n");
		peer().send(notFound, notFound.GetLength());
	}
	return 0;
}

