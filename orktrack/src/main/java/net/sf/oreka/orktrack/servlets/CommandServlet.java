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

package net.sf.oreka.orktrack.servlets;

import java.io.IOException;

import javax.servlet.ServletException;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import net.sf.oreka.messages.AsyncMessage;
import net.sf.oreka.messages.SimpleResponseMessage;
import net.sf.oreka.messages.SyncMessage;
import net.sf.oreka.orktrack.OrkTrack;
import net.sf.oreka.serializers.ServletRequestSerializer;
import net.sf.oreka.serializers.SingleLineSerializer;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;


public class CommandServlet extends HttpServlet {

    static Logger logger = LogManager.getLogger(CommandServlet.class);

    public void doGet(HttpServletRequest request, HttpServletResponse response)
            throws IOException, ServletException {
        //#####OrkObjectFactory.instance().registerOrkObject(new TestMessage());
        OrkTrack.refreshInMemoryObjects();

        ServletRequestSerializer ser = new ServletRequestSerializer();
        SingleLineSerializer ser2 = new SingleLineSerializer();

        try {
            SyncMessage obj = (SyncMessage) ser.deSerialize(request);
            AsyncMessage rsp = obj.process();
            logger.debug("Request: {}", ser2.serialize(obj));
            String resp = ser2.serialize(rsp);
            logger.debug("Response: " + resp);
            response.getWriter().println(resp);
        } catch (Throwable e) {
            logger.error("Servlet Process threw Exception",e);
            SimpleResponseMessage rsp = new SimpleResponseMessage();
            rsp.setComment(e.getMessage());
            rsp.setSuccess(false);
            try {
                String resp = ser2.serialize(rsp);
                logger.debug("Request: " + request.getQueryString() + ", Response: " + resp);
                response.getWriter().println(resp);
            } catch (Exception ae) {
                logger.error("Request: " + request.getQueryString() + ", Error:" + ae.getMessage());
            }
        }

    }
}
