package net.sf.oreka.orktrack.servlets;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import javax.servlet.*;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import java.io.IOException;

public class AccessLogFilter implements Filter {

    private static final Logger logger = LogManager.getLogger("ACCESS");
    private static final String logFormat = "%s %s %s %d %d";

    @Override
    public void init(FilterConfig filterConfig) throws ServletException {

    }

    @Override
    public void doFilter(ServletRequest servletRequest, ServletResponse servletResponse, FilterChain filterChain) throws IOException, ServletException {
        long startTime = System.currentTimeMillis();

        HttpServletRequest httpServletRequest = (HttpServletRequest)servletRequest;
        HttpServletResponse httpServletResponse = (HttpServletResponse)servletResponse;

        filterChain.doFilter (httpServletRequest, servletResponse);

        long elapsed = System.currentTimeMillis() - startTime;

        logger.info(String.format(logFormat, getIpAddr(httpServletRequest), httpServletRequest.getMethod(), getFullURL(httpServletRequest), httpServletResponse.getStatus() , elapsed  ));

    }

    @Override
    public void destroy() {

    }

    public static String getFullURL(HttpServletRequest request) {
        StringBuffer requestURL = request.getRequestURL();
        String queryString = request.getQueryString();

        if (queryString == null) {
            return requestURL.toString();
        } else {
            return requestURL.append('?').append(queryString).toString();
        }
    }

    public static String getIpAddr(HttpServletRequest request){
        String ipAddress = request.getHeader("X-FORWARDED-FOR");
        if (ipAddress == null) {
            ipAddress = request.getRemoteAddr();
        }
        return ipAddress;
    }
}
