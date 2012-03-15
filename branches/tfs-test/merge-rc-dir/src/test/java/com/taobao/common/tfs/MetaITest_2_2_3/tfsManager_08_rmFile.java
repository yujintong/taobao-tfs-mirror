package com.taobao.common.tfs.MetaITest_2_2_3;

import java.util.Random;

import org.junit.Test;
import org.junit.Ignore;

import junit.framework.Assert;

import com.taobao.common.tfs.tfsNameBaseCase;

public class tfsManager_08_rmFile extends tfsNameBaseCase 
{
	@Test
	public void test_01_rmFile_right_filePath()
	{	   
	   int Ret ;
	   log.info( "test_01_rmFile_right_filePath" );
	   tfsManager.createFile(appId, userId, "/textrmFile");
	   Ret=tfsManager.rmFile(appId, userId, "/textrmFile");
	   Assert.assertEquals("Remove File with right path should be true", Ret,0);
	}
	@Test
	public void test_02_rmFile_double_times()
	{	   
	   int Ret ;
	   log.info( "test_02_rmFile_double-times" );
	   tfsManager.createFile(appId, userId, "/textrmFile");
	   Ret=tfsManager.rmFile(appId, userId, "/textrmFile");
	   Assert.assertEquals("Remove File with right path should be true", Ret,0);
	   Ret=tfsManager.rmFile(appId, userId, "/textrmFile");
	   Assert.assertEquals("Remove File two times should be false", Ret,-14001);
	}
	@Test
	public void test_03_rmFile_not_exist()
	{	   
	   int Ret ;
	   log.info( "test_03_rmFile_not_exist" );
	   Ret=tfsManager.rmFile(appId, userId, "/textrmFile");
	   Assert.assertEquals("Remove File not exist should be false", Ret,-14001);
	}
	@Test
	public void test_04_rmFile_null_filePath()
	{	   
	   int Ret ;
	   log.info( "test_04_rmFile_null_filePath" );
	   Ret=tfsManager.rmFile(appId, userId, null);
	   Assert.assertEquals("Remove null File should be false", Ret,-14010);
	}
	@Test
	public void test_05_rmFile_empty_filePath()
	{	   
	   int Ret ;
	   log.info( "test_05_rmFile_empty_filePath" );
	   Ret=tfsManager.rmFile(appId, userId, "");
	   Assert.assertEquals("Remove empty File should be false", Ret,-14010);
	}
	@Test
	public void test_06_rmFile_wrong_filePath_1()
	{	   
	   int Ret ;
	   log.info( "test_06_rmFile_wrong_filePath_1" );
	   tfsManager.createFile(appId, userId, "/textrmFile");
	   Ret=tfsManager.rmFile(appId, userId, "textrmFile");
	   Assert.assertEquals("Remove wrong-1 File Path should be false", Ret,-14010);
	}
	@Test
	public void test_07_rmFile_wrong_filePath_2()
	{	   
	   int Ret ;
	   log.info( "test_07_rmFile_wrong_filePath_2" );
	   tfsManager.createFile(appId, userId, "/textrmFile");
	   Ret=tfsManager.rmFile(appId, userId, "///textrmFile///");
	   Assert.assertEquals("Remove wrong-2 File Path be true", Ret,0);
	}
	@Test
	public  void test_08_rmFile_wrong_filePath_3()
	{	   
	   int Ret ;
	   log.info( "test_08_rmFile_wrong_filePath_3" );
	   Ret=tfsManager.rmFile(appId, userId, "/");
	   Assert.assertEquals("Remove wrong-3 File Path should be false", Ret,1);
	}
	@Test
	public void test_09_rmFile_small_File()
	{	   
	   int Ret ;
	   log.info( "test_01_rmFile_right_filePath" );
	   tfsManager.saveFile(appId, userId, resourcesPath+"100K.jpg","/textrmFile");
	   Ret=tfsManager.rmFile(appId, userId, "/textrmFile");
	   Assert.assertEquals("Remove File with right path should be true", Ret,0);
	}
	@Test
	public void test_10_rmFile_large_File()
	{	   
	   int Ret ;
	   log.info( "test_10_rmFile_large_File" );
	   tfsManager.saveFile(appId, userId, resourcesPath+"1G.jpg","/textrmFile");
	   Ret=tfsManager.rmFile(appId, userId, "/textrmFile");
	   Assert.assertEquals("Remove File with right path should be true", Ret,0);
	}
	@Test
	public void test_11_rmFile_large_File_inanition()
	{	   
	   int Ret ;
	   log.info( "test_10_rmFile_large_File" );
	   tfsManager.saveFile(appId, userId, resourcesPath+"3M.jpg","/textrmFile");
	   int len = 1024*1024*8;
       byte[] data = new byte[len];
       Random rd = new Random();
       rd.nextBytes(data);
       tfsManager.write(appId, userId,"/textrmFile", 8*(1<<20),data,0,8*(1<<20));
	   Ret=tfsManager.rmFile(appId, userId, "/textrmFile");
	   Assert.assertEquals("Remove File with right path should be true", Ret,0);
	}
}