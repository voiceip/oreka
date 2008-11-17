package net.sf.oreka.test;

import java.util.List;

import net.sf.oreka.HibernateManager;
import net.sf.oreka.persistent.OrkProgram;

import org.hibernate.Criteria;
import org.hibernate.Session;
import org.hibernate.Transaction;

public class FillDatabaseProgram {

	static HibernateManager hibernateManager = HibernateManager.instance();	
	
	public static void main(String args[]) throws Exception
	{
		hibernateManager.configure("c:/oreka/test/mysql.hbm.xml");
		
		Session hbnSession = hibernateManager.getSession();
		Transaction tx = hbnSession.beginTransaction();
		
		OrkProgram prg = new OrkProgram();
		prg.setDescription("my program 1");
		prg.setName("myprog1");
		hbnSession.save(prg);
		
		prg = new OrkProgram();
		prg.setDescription("my program 2");
		prg.setName("myprog2");
		hbnSession.save(prg);
		
		tx.commit();
		
		List recPrograms;
		Criteria crit = hbnSession.createCriteria(OrkProgram.class);
		//crit.add( Expression.eq( "color", eg.Color.BLACK ) );
		//crit.setMaxResults(10);
		recPrograms = crit.list();
		
		
		hbnSession.close();
		
		System.out.println("Done");
	}
	
}
