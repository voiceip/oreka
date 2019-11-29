package net.sf.oreka.orkweb;

import org.apache.commons.io.IOUtils;
import org.apache.commons.lang.StringUtils;

import javax.servlet.ServletException;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.nio.file.Files;
import java.nio.file.Path;

public class TapeStream extends HttpServlet {

    private final String BASE_PATH = "/var/log/orkaudio/audio/";

    @Override
    protected void doGet(HttpServletRequest request, HttpServletResponse response)
            throws ServletException, IOException {

        String fileParam = request.getPathInfo();
        String fileName = StringUtils.removeStart(fileParam, "/");

        String absoluteFilename = BASE_PATH + fileName;
        Path path = new File(absoluteFilename).toPath();

        String mimeType = Files.probeContentType(path);
        response.setContentType(mimeType);

        OutputStream out = response.getOutputStream();
        FileInputStream in = new FileInputStream(absoluteFilename);
        IOUtils.copy(in, out);
        in.close();
        out.flush();
    }

}
