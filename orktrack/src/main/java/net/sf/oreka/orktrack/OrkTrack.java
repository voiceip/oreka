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

/**
 * 
 */
package net.sf.oreka.orktrack;

import com.codahale.metrics.JmxReporter;
import com.codahale.metrics.MetricRegistry;
import com.codahale.metrics.SharedMetricRegistries;
import net.sf.oreka.HibernateManager;
import net.sf.oreka.OrkObjectFactory;
import net.sf.oreka.orktrack.messages.*;
import org.apache.logging.log4j.Logger;

import java.util.Date;
import java.util.concurrent.TimeUnit;

import static net.sf.oreka.orktrack.Constants.*;

public class OrkTrack {

	public static HibernateManager hibernateManager = HibernateManager.instance();
	private static Date lastInMemoryObjectsSync = new Date(0);
	public static MetricRegistry METRIC_REGISTRY;
	private static JmxReporter reporter;

	private static Logger log = LogManager.getInstance().getRootLogger();

	public OrkTrack() {
		LogManager.getInstance().getConfigLogger().info("Entering OrkTrack");
	}
	
	public static void initialize(String log4jConfigFile, String hibernateConfigFile, String configFile) throws Exception {
        try {
			LogManager.getInstance().configure(log4jConfigFile);

			log.info("========================================");
			log.info(APP_NAME + " starting ...");

			// Register all OrkObjects
			OrkObjectFactory.instance().registerOrkObject(new OrkTrackConfig());
			OrkObjectFactory.instance().registerOrkObject(new MetadataMessage());
			OrkObjectFactory.instance().registerOrkObject(new TapeMessage());
			OrkObjectFactory.instance().registerOrkObject(new UserStateMessage());
			OrkObjectFactory.instance().registerOrkObject(new PingMessage());
			OrkObjectFactory.instance().registerOrkObject(new ConfigureLogMessage());
			OrkObjectFactory.instance().registerOrkObject(new InitMessage());
			ConfigManager.getInstance().load(configFile);

			hibernateManager.configure(hibernateConfigFile);

			METRIC_REGISTRY = SharedMetricRegistries.getOrCreate(DEFAULT_REGISTRY_NAME);

			reporter = JmxReporter.forRegistry(METRIC_REGISTRY)
					.convertDurationsTo(TimeUnit.MILLISECONDS)
					.convertRatesTo(TimeUnit.SECONDS)
					.inDomain("orktrack")
					.build();
			reporter.start();

		}
		catch (Throwable e) {
			log.error("OrkTrack.initialize: Error configuring Hibernate:" + e.getMessage());
			throw e ;
		}

		/*
		boolean initOk = false;
		//while(initOk == false) {
			initOk  = PortManager.instance().initialize();
			if (initOk) {
				initOk = ProgramManager.instance().load();
			}
			//try{Thread.sleep(5000);} catch (Exception e) {};
		//}
		 */
		refreshInMemoryObjects();

		log.info(APP_NAME + " started successfully.");
		log.info("----------------------------------------");

		METRIC_REGISTRY.counter(APP_NAME).inc();
	}
	
	public static void refreshInMemoryObjects() {
		Date now = new Date();
		if((now.getTime() - lastInMemoryObjectsSync.getTime()) > 5000) {
			LogManager.getInstance().getRecurrentLogger().debug("Refreshing In-Memory objects");
			// refresh every 5 seconds
			lastInMemoryObjectsSync = now;
			PortManager.instance().initialize();
			ProgramManager.instance().load();
		}
	}
	
}
