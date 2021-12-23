/*
 * Oreka -- A media capture and retrieval platform
 * 
 * Copyright (C) 2013, Orecx LLC
 *
 * http://www.orecx.com
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License.
 * Please refer to http://www.gnu.org/copyleft/gpl.html
 *
 */
package net.sf.oreka.util;

import java.io.File;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;

import org.apache.log4j.Logger;
import org.apache.log4j.PropertyConfigurator;
import org.w3c.dom.Document;
import org.w3c.dom.NamedNodeMap;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;

public class TomcatServerXMLParser {

	static Logger logger = Logger.getLogger(TomcatServerXMLParser.class);
	private static String serverTcpPort = null;
	private static String audioContextPath = "";
	private static String audioFilePath = "";
	private static String scheme = null;
	
	private static String DEFAULT_TCPPORT = "8080";
	private static String DEFAULT_HOSTNAME = "localhost";
	private static String DEFAULT_SCHEME = "http";
	private static String tomcatHome = "";
	private static String defaultRecordingPath = "";
	
	private static final String TOMCAT_RECORDINGS_PATH = "/webapps/ROOT/";

	public static String getOrkTrackTcpPort(){

		// Default to 8080
		if (serverTcpPort == null)
			serverTcpPort = "8080";

		return serverTcpPort;
	}

	public static String getOrkWebTcpPort(){

		// Default to 8080
		if (serverTcpPort == null)
			serverTcpPort = "8080";

		return serverTcpPort;
	}
	
	public static String getTomcatTcpPort(){

		if (serverTcpPort == null)
			return DEFAULT_TCPPORT;
					
		return serverTcpPort;
	}

	// FOR NOW USE DEFAULTS
	public static String getOrkTrackHostName(){
				
// 		if (serverHostName == null)
//			return DEFAULT_HOSTNAME;

 		return DEFAULT_HOSTNAME;
	}

	public static String getOrkWebHostName(){

// 		if (serverHostName == null)
//			return DEFAULT_HOSTNAME;

		return DEFAULT_HOSTNAME;
	}

	public static String getScheme(){

		if (scheme==null)
			return DEFAULT_SCHEME;
				
		return scheme;
	}

	public static String getStartURLFromTomcatConfig(){		
		return getScheme() + "://" + getOrkWebHostName() + ":" + getOrkWebTcpPort() + "/"; 
	}

	// Assumes parseServerXML was already called by ContextListener at tomcat start
	public static String getAudioContextPath(){
		if (audioContextPath==null)
			audioContextPath="";
		return audioContextPath;
	}

	// Assumes parseServerXML was already called by ContextListener at tomcat start
	public static String getAudioFilePath() {
		if (audioFilePath==null)
			audioFilePath="";
		return audioFilePath;
	}

	public static String getDefaultRecordingPath() {
		return defaultRecordingPath;
	}

	public static void setDefaultRecordingPath(String defaultRecordingPath) {
		TomcatServerXMLParser.defaultRecordingPath = defaultRecordingPath;
	}

	public static String getTomcatHome() {
		return tomcatHome;
	}
	
	public static void setTomcatHome(String tomcatHome) {
		if (tomcatHome!=null && !tomcatHome.equals("")) {
			TomcatServerXMLParser.tomcatHome = tomcatHome; 
			setDefaultRecordingPath(tomcatHome + TOMCAT_RECORDINGS_PATH);
		} else {
			tomcatHome = "";
			setDefaultRecordingPath("");
		}	
	}

	// Parse server.xml file 
	public static void parseServerXML(String tomcatHome) throws Exception {

		if (tomcatHome == null || tomcatHome.equals("")){
			if (logger.isDebugEnabled())
				logger.debug("parseServerXML() - no server.xml provided, will use defaults");
			return;
		}
		
		String serverXmlPath = tomcatHome + "/conf/server.xml";
		
		if (logger.isDebugEnabled())
			logger.debug("parseServerXML() - parsing Tomcat server.xml file, path: " + serverXmlPath);
			
		DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
		DocumentBuilder builder = factory.newDocumentBuilder();
		
		File serverConf = new File(serverXmlPath);		
		Document document = builder.parse(serverConf);
		document.getDocumentElement().normalize();
		
		// Find Server node
		Node serverNode = document.getLastChild();	
		NodeList serverNodeChildren = serverNode.getChildNodes();
		
		// Find Service node
		Node serviceNode = null;		
		for (int i=0;i<serverNodeChildren.getLength();i++){
			
			Node node =  serverNodeChildren.item(i);
			
			if (node.getNodeType() == Node.ELEMENT_NODE && node.getNodeName().equals("Service")){
				serviceNode = node;

				if (logger.isDebugEnabled())
					logger.debug("parseServerXML() - Service found=" + node.getNodeName());
				break;
			}
		}
		
		// The Service node has two HTTP Connector nodes (regular and SSL)
		NodeList serviceNodeChildren = serviceNode.getChildNodes();
		
		Node node = null;
		Node engineNode = null;
		boolean httpConnectorFound = false;
		boolean SSLEnabled = false;
		
		if (serviceNodeChildren==null) {
			logger.error("parseServerXML() no Service node children found!? This should not happen.");			
		} 
		else {
		
			for (int i=0;i<serviceNodeChildren.getLength();i++){
				
				node =  serviceNodeChildren.item(i);
				
				// Get HTTP connector, preferably one that does not have SSL enabled
				if (!httpConnectorFound || (httpConnectorFound && SSLEnabled) &&  
					node!=null && node.getNodeType() == Node.ELEMENT_NODE && node.getNodeName().equals("Connector")){	
					
					// Get all the attributes
					boolean isHttpConnector = false;
					boolean isSSLEnabled = false;
					Integer tcpPortIndex = null;
					Integer schemeIndex = null;
					NamedNodeMap connectorAttributes = node.getAttributes();
					if (connectorAttributes==null)
						continue;
					
					for (int j=0;j<connectorAttributes.getLength();j++) {
						
						Node attributeNode =  connectorAttributes.item(j);
							
						if (attributeNode!=null && attributeNode.getNodeType() == Node.ATTRIBUTE_NODE){
							
							if (attributeNode.getNodeName()==null || attributeNode.getNodeName().length()==0)
								continue;
							
							if (logger.isDebugEnabled())
								logger.debug("Service attribute index -  " + j + " - name: " + attributeNode.getNodeName() + " - value: " + attributeNode.getNodeValue());
							
							// For Tomcat 7.0
							if (attributeNode.getNodeName().equals("protocol")) {
								String protocol = attributeNode.getNodeValue();
								if (protocol!=null && protocol.equalsIgnoreCase("HTTP/1.1"))
									isHttpConnector = true;						
							}	
							
							// For Tomcat 5.5
							if (attributeNode.getNodeName().equals("maxHttpHeaderSize"))
								isHttpConnector = true;						
	
							// For Tomcat 5.5 and 7.0
							if (attributeNode.getNodeName().equals("SSLEnabled")) {
								String ssl = attributeNode.getNodeValue();
								if (ssl!=null && ssl.equalsIgnoreCase("true"))
									isSSLEnabled = true;						
							}	
	
							if (attributeNode.getNodeName().equals("port"))
								tcpPortIndex = j;
							
							if (attributeNode.getNodeName().equals("scheme"))
								schemeIndex = j;
						}
					}
					// Evaluate attributes before going to next node
					// If both true, that's the correct connector, get the port
					if (isHttpConnector) {							
						serverTcpPort = tcpPortIndex!=null ? connectorAttributes.item(tcpPortIndex).getNodeValue() : null;
						scheme = schemeIndex!=null ? connectorAttributes.item(schemeIndex).getNodeValue() : null; 
						httpConnectorFound = true;
						SSLEnabled = isSSLEnabled;
						continue;
					}
				}
				
				// Get the Engine node
				if (node.getNodeType() == Node.ELEMENT_NODE && node.getNodeName().equals("Engine")){	
					engineNode = node;
					break;
				}
			}
		}
		
		// Find Host node under Engine node
		if (engineNode!=null) {
		
			NodeList engineNodeChildren = engineNode.getChildNodes();
			Node hostNode = null;
			
			for (int i=0;i<engineNodeChildren.getLength();i++){
					
				node =  engineNodeChildren.item(i);
				if (node.getNodeType() == Node.ELEMENT_NODE && node.getNodeName().equalsIgnoreCase("Host")){
					hostNode = node;
					break;
				}
			}	
		
			// Find Audio and Screen context paths, store aliases in a HashMap
			if (hostNode!=null) {

				NodeList hostNodeChildren = hostNode.getChildNodes();
				Node contextNode1 = null;
				Node contextNode2 = null;
				
				for (int i=0;i<hostNodeChildren.getLength();i++){
					
					node =  hostNodeChildren.item(i);
					if (node.getNodeType() == Node.ELEMENT_NODE && node.getNodeName().equalsIgnoreCase("Context")){
						if (contextNode1==null) {
							contextNode1 = node;
						} else if (contextNode2==null) {
							contextNode2 = node;
							break;
						}
					}
				}			
				getContextPath(contextNode1);
				getContextPath(contextNode2);
				
			}
		}
		
		if (logger.isDebugEnabled()) { 
			logger.debug("parseServerXML() serverTcpPort=" + serverTcpPort + " scheme=" + scheme + " audioContextPath=" + audioContextPath);
		}			     
	}

	// Get the context path for contextNode
	public static void getContextPath(Node contextNode) {

		if (contextNode == null)
			return;
		
		String contextType = ""; 
		
		NamedNodeMap hostAttributes = contextNode.getAttributes();
		for (int j=0;j<hostAttributes.getLength();j++){
			
			// Get context path
			if (hostAttributes.item(j).getNodeType() == Node.ATTRIBUTE_NODE && contextType.equals("")){
				
				if (logger.isDebugEnabled())
					logger.debug("getContextPath() - Context attribute index -  " + j + " - name: " + hostAttributes.item(j).getNodeName() + " - value: " + hostAttributes.item(j).getNodeValue());		
				
				if (hostAttributes.item(j).getNodeName().equals("path")) {
					if (hostAttributes.item(j).getNodeValue().contains("audio")) {
						audioContextPath = hostAttributes.item(j).getNodeValue();
						contextType = "audio";
					} 
				}
			}
			// If a context path was found, look for its docBase
			if (!contextType.equals("")) {

				for (int k=0;k<hostAttributes.getLength();k++){
					if (hostAttributes.item(k)!=null && hostAttributes.item(k).getNodeName().contains("docBase")) {
						if (contextType.equals("audio"))
							audioFilePath = hostAttributes.item(k).getNodeValue();
						break;
					}
				}
			}	
		}
	}

	// For testing purposes
	public static void main(String args[]) throws Exception
	{
		System.out.println("Entering TomcatServerXML main...");

		// Attempt to configure log4j
		File file = new File("C:/oreka/logging.properties");
		if (file.exists())
			PropertyConfigurator.configure("C:/oreka/logging.properties");
		else
			System.out.println("Error Configuration Log4j");

		Logger logger = Logger.getLogger(TomcatServerXMLParser.class);
		logger.info("Main () ");

		String tomcatHome = "C:/Program Files/Apache Software Foundation/Tomcat 7.0";
		parseServerXML(tomcatHome);

		System.out.println("OrkWeb port: " + getOrkWebTcpPort() + " - OrkTrack port: " + getOrkTrackTcpPort() + " scheme=" + getScheme());
		System.out.println("Audio Context: " + getAudioContextPath());
		System.out.println("Audio Path: " + getAudioFilePath());

	}


}
