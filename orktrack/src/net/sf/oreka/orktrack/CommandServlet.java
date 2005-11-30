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

package net.sf.oreka.orktrack;

import java.io.IOException;
import java.io.PrintWriter;

import javax.servlet.ServletException;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import net.sf.oreka.OrkException;
import net.sf.oreka.OrkObjectFactory;
import net.sf.oreka.messages.AsyncMessage;
import net.sf.oreka.messages.SimpleResponseMessage;
import net.sf.oreka.messages.SyncMessage;
import net.sf.oreka.messages.test.TestMessage;
import net.sf.oreka.serializers.ServletRequestSerializer;
import net.sf.oreka.serializers.SingleLineSerializer;

import org.apache.log4j.Logger;

public class CommandServlet extends HttpServlet {
	
	static Logger logger = Logger.getLogger(CommandServlet.class);		
	
    public void doGet(HttpServletRequest request, HttpServletResponse response)
    throws IOException, ServletException
    {
		//#####OrkObjectFactory.instance().registerOrkObject(new TestMessage());
		OrkTrack.refreshInMemoryObjects();
		
		ServletRequestSerializer ser = new ServletRequestSerializer();
		
		try {
			SyncMessage obj = (SyncMessage)ser.deSerialize(request);
			AsyncMessage rsp = obj.process();
			SingleLineSerializer ser2 = new SingleLineSerializer();
			String req = ser2.serialize(obj);
			logger.debug("Request: " + req);			
			PrintWriter out = response.getWriter();
			String resp = ser2.serialize(rsp);
			logger.debug("Reponse: " + resp);
			out.println(resp);
		}
		catch (Exception e) {
			logger.debug("Request: " + request.getQueryString());
			SimpleResponseMessage rsp = new SimpleResponseMessage();
			rsp.setComment(e.getMessage());
			rsp.setSuccess(false);
			SingleLineSerializer ser2 = new SingleLineSerializer();
			PrintWriter out = response.getWriter();
			try {
				String resp = ser2.serialize(rsp);
				logger.debug("Reponse: " + resp);
				out.println(resp);
			}
			catch (Exception ae) {
				logger.error("Error:" + ae.getMessage());
			}
		}
		
//		SingleLineSerializer ser2 = new SingleLineSerializer();
//		try {
//			OrkObject obj = ser.deSerialize(request);
//	        response.setContentType("text/html");
//	        PrintWriter out = response.getWriter();
//	        out.println("<html>");
//	        out.println("<head>");
//	        out.println("<title>Sucess</title>");
//	        out.println("</head>");
//	        out.println("<body>");
//	        out.println("<h1>");
//	        out.println(ser2.serialize(obj));
//	        out.println("</h1>");
//	        out.println("</body>");
//	        out.println("</html>");
//		}
//		catch (OrkException e) {
//	        response.setContentType("text/html");
//	        PrintWriter out = response.getWriter();
//	        out.println("<html>");
//	        out.println("<head>");
//	        out.println("<title>Error</title>");
//	        out.println("</head>");
//	        out.println("<body>");
//	        out.println("<h1>");
//	        out.println(e.getMessage());
//	        out.println("</h1>");
//	        out.println("</body>");
//	        out.println("</html>");
//		}
    	
//		logger.log(Level.INFO, request.getPathInfo());
//		logger.log(Level.INFO, request.getRequestURL());
//		logger.log(Level.INFO, request.getQueryString());
//    	logger.log(Level.INFO, request.getParameter("toto"));
//    	logger.log(Level.INFO, request.getParameter("titi"));  
//    	
//        response.setContentType("text/html");
//        PrintWriter out = response.getWriter();
//        out.println("<html>");
//        out.println("<head>");
//        out.println("<title>Hello World!</title>");
//        out.println("</head>");
//        out.println("<body>");
//        out.println("<h1>Hello World!</h1>");
//        out.println("</body>");
//        out.println("</html>");
    }
}
