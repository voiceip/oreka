/*
 * Oreka -- A media capture and retrieval platform
 *
 * Copyright (C) 2017, orecx LLC
 *
 * http://www.orecx.com
 *
 */
package net.sf.oreka.orktrack.test;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.MalformedURLException;
import java.net.URL;

public class InitMessageGenerator {
	private static StringBuffer msg = new StringBuffer();
	private static String hostName = "localhost";

	public static void main(String[] args) {

		InitMessageGenerator initMessageGenerator = new InitMessageGenerator();
		initMessageGenerator.setHostName("localhost");

		initMessageGenerator.sendURLRequest(initMessageGenerator.generateInitMessage());
	}

	public void sendURLRequest(String requestString) {

		// http://localhost:8080/orktrack/command?cmd=init

		String responseLine = "";
		StringBuffer responseSB = new StringBuffer();

		try {
			URL url = new URL(requestString);
			BufferedReader in = new BufferedReader(new InputStreamReader(url.openStream()));

			while ((responseLine = in.readLine()) != null) {
				responseSB.append(responseLine);
			}
			in.close();

		} catch (MalformedURLException e) {
			e.printStackTrace();
		} catch (IOException e) {
			e.printStackTrace();
		}
		System.out.println(responseSB.toString());
	}

	private StringBuffer getMessageBody() {

		msg = new StringBuffer();
		msg.append("http://").append(hostName).append(":8080/orktrack/command?cmd=init");
		return msg;
	}

	public String generateInitMessage() {

		StringBuffer sb = getMessageBody();

		System.out.println("Request: " + sb.toString());

		return sb.toString();
	}
	
	private void setHostName(String hostName) {
		InitMessageGenerator.hostName = hostName;
	}
}
