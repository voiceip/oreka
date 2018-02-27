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

import org.hibernate.cfg.Configuration;
import org.hibernate.SessionFactory;
import org.hibernate.Session;
import org.hibernate.Transaction;
import org.hibernate.HibernateException;
//import org.hibernate.tool.hbm2ddl.SchemaExport;
//import org.hibernate.ScrollableResults;

public class HbnXmlTest {
    
    /** Creates a new instance of TestHibernate */
    public HbnXmlTest() {
    }
    
    /**
     * @param args the command line arguments
     */
    public static void main(String[] args) {
		
		Configuration config = new Configuration()
			.configure("mysql.hbm.xml");
			
		config.addClass(HbnXmlTestClass.class);
        SessionFactory sessions = config.buildSessionFactory();
        Session session = sessions.openSession();

        Transaction tx = null;

        try {
			/*
            SchemaExport export = new SchemaExport(config);
            export.setOutputFile("c:\\schema.sql");
            export.create(true,false);
            */
        	
			for (int i=0; i<100 ; i++)
			{
	            tx = session.beginTransaction();

	            HbnXmlTestClass obj = new HbnXmlTestClass();
	            
				session.save(obj);				
	            tx.commit();
			}       	
        	
        	
			// insert
        	/*
			for (int i=0; i<100 ; i++)
			{
	            tx = session.beginTransaction();

				RecSession RecSession = new RecSession();
				RecSession.setDuration(i);
				
				
				RecSegment intr1 = new RecSegment();
				intr1.setRecSession(RecSession);
				intr1.setLocalParty(i + " " + 1);
				intr1.setDuration(23);
				RecSegment intr2 = new RecSegment();
				intr2.setRecSession(RecSession);
				intr2.setLocalParty(i + " " + 2);
				intr2.setDuration(45);
				
				session.save(RecSession);
				session.save(intr1);
				session.save(intr2);				
	            tx.commit();
			}
*/
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
			
			// Many to many select
			/*
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
			// one to one create
            tx = session.beginTransaction();

			RecProgram prog = new RecProgram();			
			User user = new User();
			prog.setTargetUser(user);
			
			session.save(user);
			session.save(prog);	
            tx.commit();
			*/
			/*
			// one to one select
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
        }
        catch ( HibernateException he ) {
            throw he;
        }
        finally {
            session.close();
        }
    }
    
}
