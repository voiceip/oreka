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

package net.sf.oreka.persistent;



import java.util.Date;
import java.util.GregorianCalendar;
import java.util.Iterator;

import org.hibernate.HibernateException;
import org.hibernate.Session;
import org.hibernate.SessionFactory;
import org.hibernate.Transaction;
import org.hibernate.cfg.AnnotationConfiguration;
//import org.hibernate.tool.hbm2ddl.SchemaExport;
//import org.hibernate.ScrollableResults;

public class AnnotationsTest {
    
    /** Creates a new instance of TestHibernate */
    public AnnotationsTest() {
    }
    
    /**
     * @param args the command line arguments
     */
    public static void main(String[] args) {
		
//    	Service srv1 = new Service();
//    	srv1.setHostname("aoum");
//    	
//    	srv1.setServiceClass(Service.ServiceClass.unkn);
//    	
//		Method method = null;
//		Object[] values = null;
//		try {
//			Class myClass = Service.ServiceClass.class;
//			method = myClass.getDeclaredMethod( "values", new Class[0] );
//			values = (Object[]) method.invoke(null, new Object[0] );
//		}
//		catch (Exception e) {
//			int i = 0;
//		}
		
//		if(true) {return;} 
    	
//    	Date date = new Date();
//    	long mili = date.getTime();
//    	int sec = (int)(mili/1000);
//    	long mili2 = (long)sec*1000;
//    	Date date2 = new Date(mili2);
    	
    	AnnotationConfiguration config = (AnnotationConfiguration)new AnnotationConfiguration();
//    	    .setProperty("hibernate.dialect", "org.hibernate.dialect.HSQLDialect")
//            .setProperty("hibernate.connection.driver_class", "org.hsqldb.jdbcDriver")
//            .setProperty("hibernate.connection.url", "jdbc:hsqldb:mem:test")
//            .setProperty("hibernate.connection.username", "sa")
//            .setProperty("hibernate.connection.password", "")
//            .setProperty("hibernate.connection.pool_size", "1")
//            .setProperty("hibernate.connection.autocommit", "true")
//            .setProperty("hibernate.cache.provider_class", "org.hibernate.cache.HashtableCacheProvider")
//            .setProperty("hibernate.hbm2ddl.auto", "create-drop")
//            .setProperty("hibernate.show_sql", "true");
//    	config.configure("mysql.hbm.xml");
    	config.configure("hsqldb.hbm.xml");
    	
    	config.addAnnotatedClass(AnnotatedTestClass.class);
		config.addAnnotatedClass(OrkProgram.class);
		config.addAnnotatedClass(OrkSession.class);
		config.addAnnotatedClass(OrkSegment.class);
		config.addAnnotatedClass(OrkTape.class);
		config.addAnnotatedClass(OrkUser.class);
		config.addAnnotatedClass(OrkLoginString.class);
		config.addAnnotatedClass(OrkDomain.class);		
		config.addAnnotatedClass(OrkService.class);
		config.addAnnotatedClass(OrkPort.class);
		config.addAnnotatedClass(OrkPortFace.class);
    	
        SessionFactory sessions = config.buildSessionFactory();
        Session session = sessions.openSession();

        Transaction tx = null;

        try {
			/*
            SchemaExport export = new SchemaExport(config);
            export.setOutputFile("c:\\schema.sql");
            export.create(true,false);
            */
        	
        	// single insert
//            tx = session.beginTransaction();
//            AnnotatedTestClass obj = new AnnotatedTestClass();
//            obj.setMyEnum(TestEnum.value2);
//			session.save(obj);				
//            tx.commit();
        	
			// insert
//			for (int i=0; i<100 ; i++)
//			{
//	            tx = session.beginTransaction();
//	            Domain obj = new Domain();
//				session.save(obj);				
//	            tx.commit();
//			}

			/*
			// iterator select
			Iterator documents = session.createQuery(
            	"from Documents doc join doc.car")
            	.list()
            	.iterator();

			while ( documents.hasNext() ) {
			    Object[] row = (Object[]) documents.next();
			    Documents doc  = (Documents)row[0];
			    Car car  = (Car)row[1];
				
				//Documents doc = (Documents)documents.next();
				
				System.out.println(doc.getId() + " " + car.getId());
			}
			*/
			/*
			// scrollable select
			ScrollableResults scrollDocs = session.createQuery(
        		"from RecSegment intr join intr.RecSession").scroll();
			if ( scrollDocs.last() ) {
				System.out.println("Num res:" + scrollDocs.getRowNumber() + "\n\n");	
			}
			
			scrollDocs.beforeFirst();
			while (scrollDocs.next())
			{
				RecSegment doc  = (RecSegment)scrollDocs.get(0);
				RecSession car  = (RecSession)scrollDocs.get(1);	
				System.out.println(doc.getId() + " " + car.getId());			
			}
			*/
			
			/*
			// many to many insert
            tx = session.beginTransaction();

			RecProgram prog1 = new RecProgram();
			RecProgram prog2 = new RecProgram();			
			RecSegment seg = new RecSegment();
			
			HashSet programs = new HashSet();
			programs.add(prog1);
			programs.add(prog2);
			seg.setRecPrograms(programs);
			
			session.save(prog1);
			session.save(prog2);
			session.save(seg);				
            tx.commit();
            */
			/*
			// Many to many select
			ScrollableResults scrollDocs = session.createQuery(
			"from RecSegment as seg join seg.recPrograms as prg where prg.id=2").scroll();
			if ( scrollDocs.last() ) {
				System.out.println("Num res:" + scrollDocs.getRowNumber() + "\n\n");	
			}
			
			scrollDocs.beforeFirst();
			while (scrollDocs.next())
			{
				RecSegment seg  = (RecSegment)scrollDocs.get(0);
				RecProgram prg  = (RecProgram)scrollDocs.get(1);	
				System.out.println(seg.getId() + " " + prg.getId());			
			}
			*/
			
			/*
			// one to many create
            tx = session.beginTransaction();

			RecProgram prog = new RecProgram();			
			User user = new User();
			prog.setTargetUser(user);
			
			session.save(user);
			session.save(prog);	
            tx.commit();
			*/
			/*
			// one to many select
			ScrollableResults scrollDocs = session.createQuery(
			"from RecProgram as prg join prg.targetUser as tgt").scroll();
			if ( scrollDocs.last() ) {
				System.out.println("Num res:" + scrollDocs.getRowNumber() + "\n\n");	
			}
			
			scrollDocs.beforeFirst();
			while (scrollDocs.next())
			{
				RecProgram prg  = (RecProgram)scrollDocs.get(0);	
				User tgt  = (User)scrollDocs.get(1);
				System.out.println(prg.getId() + " " + tgt.getId());			
			}
			*/		
        	
        	// multi-session stuff
//          tx = session.beginTransaction();
//          RecTape obj = new RecTape();
//          Service srv = new Service();
//          obj.setService(srv);
//          srv.setName("service1");
//          session.save(srv);
//          session.save(obj);				
//          tx.commit();
//          session.close();
//          
//          session = sessions.openSession();
//          tx = session.beginTransaction();
//          obj = new RecTape();
//          obj.setService(srv);
//          session.save(obj);
//          tx.commit();
//          session.close();
        	
//        	tx = session.beginTransaction();
//        	//Service.retrieveByName("toto", session);
//        	RecSegment seg = new RecSegment();
//        	seg.setLoginString("too");
//        	seg.setDuration(56);
//        	seg.setDirection(Direction.in);
//        	seg.setRemoteParty("135");
//        	session.save(seg);
//        	
//        	java.util.List recSegments = session.createQuery(
//            "from RecSegment as seg where seg.remoteParty = :rp")
//            .setString("rp", "135")
//            .list();
//        	if (recSegments.size() > 0) {
//        		RecSegment result = (RecSegment)recSegments.get(0);
//        		System.out.println(result.getLoginString() + " " + result.getDirection().name());
//        	}
        	
        	// enum stuff
//        	tx = session.beginTransaction();
//        	//Service.retrieveByName("toto", session);
//        	Service srv = new Service();
//        	srv.setHostname("aoum");
//        	srv.setServiceClass(ServiceClass.audio);
//        	session.save(srv);
//        	
//        	java.util.List services = session.createQuery(
//            "from Service as srv where srv.hostname = :rp")
//            .setString("rp", "aoum")
//            .list();
//        	if (services.size() > 0) {
//        		Service result = (Service)services.get(0);
//        		System.out.println(result.getHostname() + result.getServiceClass().name());
//        	}
        	


//			tx = session.beginTransaction();
//			
//			Iterator portFaces = session.createQuery(
//        	"from RecPortFace")
//        	.list()
//        	.iterator();
        	
        	tx = session.beginTransaction();
        	System.out.println("hello");
        	OrkSegment seg = new OrkSegment();
         	GregorianCalendar cal = new GregorianCalendar();
        	Date now = cal.getTime();
         	seg.setTimestamp(now);
         	session.save(seg);
        
         	GregorianCalendar cal2 = new GregorianCalendar();
         	cal2.setTimeInMillis(cal.getTimeInMillis() + 1000);
         	
    		Iterator segments = session.createQuery(
	    	"from RecSegment as seg where seg.timestamp= :date")
	    	//.setDate("date", now)
	    	//.setDate("date2", new Date(time+10000000))
	    	.setCalendar("date", cal)
	    	.list()
	    	.iterator();
		if(segments.hasNext()) {
			OrkSegment seg2 = (OrkSegment)segments.next();
			System.out.println("db timestamp:" + seg2.getTimestamp().getTime() + " my:" + now.getTime());
		}
		else {
			System.out.println("fail");
		}
        	
        }
        catch ( HibernateException he ) {
            throw he;
        }
        finally {
            session.close();
        }
    }
    
}