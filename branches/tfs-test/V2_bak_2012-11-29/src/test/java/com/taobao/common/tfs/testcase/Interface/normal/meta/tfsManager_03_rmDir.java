package com.taobao.common.tfs.testcase.Interface.normal.meta;


import org.junit.Test;
import org.junit.Ignore;

import com.taobao.common.tfs.testcase.metaTfsBaseCase;

import junit.framework.Assert;



public class tfsManager_03_rmDir extends metaTfsBaseCase 
{
	@Test
	public void test_01_rmDir_right_filePath()
	{	   
	   int Ret ;
	   log.info(new Throwable().getStackTrace()[0].getMethodName());
	   tfsManager.createDir(appId, userId, "/textrmDir");
	   Ret=tfsManager.rmDir(appId, userId, "/textrmDir");
	   Assert.assertEquals("Remove Dir with right path should be true", Ret,0);
	}
	@Test
	public void test_02_rmDir_double_times()
	{	   
	   int Ret ;
	   log.info(new Throwable().getStackTrace()[0].getMethodName());
	   tfsManager.createDir(appId, userId, "/textrmDir");
	   Ret=tfsManager.rmDir(appId, userId, "/textrmDir");
	   Assert.assertEquals("Remove Dir with right path should be true", Ret,0);
	   Ret=tfsManager.rmDir(appId, userId, "/textrmDir");
	   Assert.assertEquals("Remove Dir two times should be falae", Ret,-14001);
	}
	@Test
	public void test_03_rmDir_not_exist_filePath()
	{	   
	   int Ret ;
	   log.info(new Throwable().getStackTrace()[0].getMethodName());
	   Ret=tfsManager.rmDir(appId, userId, "/textrmDir");
	   Assert.assertEquals("Remove Dir not exist should be false", Ret,-14001);
	}
	@Test
	public void test_04_rmDir_null_filePath()
	{	   
	   int Ret ;
	   log.info(new Throwable().getStackTrace()[0].getMethodName());
	   Ret=tfsManager.rmDir(appId, userId, null);
	   Assert.assertEquals("Remove Dir null should be falae", Ret,-14010);
	}
	@Test
	public void test_05_rmDir_empty_filePath()
	{	   
	   int Ret ;
	   log.info(new Throwable().getStackTrace()[0].getMethodName());
	   Ret=tfsManager.rmDir(appId, userId, "");
	   Assert.assertEquals("Remove Dir empty should be falae", Ret,-14010);
	}
	@Test
	public void test_06_rmDir_wrong_filePath_1()
	{	   
	   int Ret ;
	   log.info(new Throwable().getStackTrace()[0].getMethodName());
	   tfsManager.createDir(appId, userId, "/textrmDir");
	   Ret=tfsManager.rmDir(appId, userId, "textrmDir");
	   Assert.assertEquals("Remove wrong Dir should be falae", Ret,-14010);
	   tfsManager.rmDir(appId, userId, "/textrmDir");
	}
	@Test
	public void test_07_rmDir_wrong_filePath_2()
	{	   
	   int Ret ;
	   log.info(new Throwable().getStackTrace()[0].getMethodName());
	   Ret=tfsManager.rmDir(appId, userId, "/");
	   Assert.assertEquals("Remove wrong Dir should be falae", Ret,-14017);
	}
	@Test
	public void test_08_rmDir_wrong_filePath_3()
	{	   
	   int Ret ;
	   log.info(new Throwable().getStackTrace()[0].getMethodName());
	   tfsManager.createDir(appId, userId, "/textrmDir");
	   Ret=tfsManager.rmDir(appId, userId, "////textrmDir/////");
	   Assert.assertEquals("Remove wrong Dir be true", Ret,0);
	}
	@Test
	public  void test_09_rmDir_filePath_with_File()
	{	   
	   int Ret ;
	   log.info(new Throwable().getStackTrace()[0].getMethodName());
	   tfsManager.createDir(appId, userId, "/textrmDir");
	   tfsManager.createFile(appId, userId, "/textrmDir/textrmDir");
	   Ret=tfsManager.rmDir(appId, userId, "/textrmDir");
	   Assert.assertEquals("Remove Dir with File should be falae", Ret,-14003);
	   tfsManager.rmFile(appId, userId, "/textrmDir/textrmDir");
	   tfsManager.rmDir(appId, userId, "/textrmDir");
	}
	@Test
	public void test_10_rmDir_filePath_with_Dir()
	{	   
	   int Ret ;
	   log.info(new Throwable().getStackTrace()[0].getMethodName());
	   tfsManager.createDir(appId, userId, "/textrmDir");
	   tfsManager.createDir(appId, userId, "/textrmDir/textrmDir");
	   Ret=tfsManager.rmDir(appId, userId, "/textrmDir");
	   Assert.assertEquals("Remove Dir with Dir should be falae", Ret,-14003);
	   tfsManager.rmDir(appId, userId, "/textrmDir/textrmDir");
	   Ret=tfsManager.rmDir(appId, userId, "/textrmDir");
	}
}
