package net.sf.oreka.orkweb;

import lombok.extern.log4j.Log4j;
import lombok.extern.slf4j.Slf4j;
import org.apache.commons.io.IOUtils;
import org.apache.commons.lang.StringUtils;
import org.apache.logging.log4j.Logger;

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
    private Logger logger = LogManager.getInstance().getRootLogger();

    @Override
    protected void doGet(HttpServletRequest request, HttpServletResponse response)
            throws ServletException, IOException {

        String fileParam = request.getPathInfo();
        String fileName = StringUtils.removeStart(fileParam, "/");

        String absoluteFilename = BASE_PATH + fileName;
        Path path = new File(absoluteFilename).toPath();

        logger.info("Serve Media File File {}", absoluteFilename);

        String mimeType = Files.probeContentType(path);
        response.setContentType(mimeType);
        response.setHeader("Content-Disposition", "inline;filename=" + path.getFileName().toString());

        OutputStream out = response.getOutputStream();
        FileInputStream in = new FileInputStream(absoluteFilename);
        IOUtils.copy(in, out);
        in.close();
        out.flush();
    }

}
