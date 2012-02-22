package com.taobao.common.tfs.itest;

import org.junit.Test;
import org.junit.Ignore;

import junit.framework.Assert;

import com.taobao.common.tfs.tfsNameBaseCase;

public class tfsManager_07_mvFile extends tfsNameBaseCase 
{
	@Test
	public void test_01_mvFile_right()
	{
	   boolean bRet;
	   log.info( "test_01_mvFile_right" );
	   bRet=tfsManager.createFile(appId, userId, "/textmvFile1src");
       Assert.assertTrue(bRet);
	   bRet=tfsManager.mvFile(appId, userId, "/textmvFile1src", "/textmvFile1tar");
       Assert.assertTrue("mvFile with right path should be true", bRet);
       tfsManager.rmFile(appId, userId, "/textmvFile1tar");
	   bRet=tfsManager.rmFile(appId, userId, "/textmvFile1src");
	   Assert.assertFalse("mvFile with remove path should be false", bRet);
	}
	@Test
	public void test_02_mvFile_null_destFilePath()
	{
	   boolean bRet;
	   log.info( "test_02_mvFile_null_destFilePath" );
	   bRet=tfsManager.createFile(appId, userId, "/textmvFile1src");
       Assert.assertTrue(bRet);
	   bRet=tfsManager.mvFile(appId, userId, "/textmvFile1src",null);
       Assert.assertFalse("mvFile with null destFilePath should be false", bRet);
       bRet=tfsManager.rmFile(appId, userId, "/textmvFile1src");
       Assert.assertTrue(bRet);
	}
	@Test
	public void test_03_mvFile_empty_destFilePath()
	{
	   boolean bRet;
	   log.info( "test_03_mvFile_empty_destFilePath" );
	   bRet=tfsManager.createFile(appId, userId, "/text1");
	   bRet=tfsManager.mvFile(appId, userId, "/text1","");
       Assert.assertFalse("mvFile with empty destFilePath should be false", bRet);
       bRet=tfsManager.rmFile(appId, userId, "/text1");
	}
	@Test
	public void test_04_mvFile_wrong_destFilePath_1()
	{
	   boolean bRet;
	   log.info( "test_04_mvFile_wrong_destFilePath_1" );
	   bRet=tfsManager.createFile(appId, userId, "/text1");
	   bRet=tfsManager.mvFile(appId, userId, "/text1","/");
       Assert.assertFalse("mvFile with wrong_1 destFilePath should be false", bRet);
       bRet=tfsManager.rmFile(appId, userId, "/text1");
	}
	@Test
	public void test_05_mvFile_wrong_destFilePath_2()
	{
	   boolean bRet;
	   log.info( "test_05_mvFile_wrong_destFilePath_2" );
	   bRet=tfsManager.createFile(appId, userId, "/text1");
	   bRet=tfsManager.mvFile(appId, userId, "/text1","text2");
       Assert.assertFalse("mvFile with wrong_2 destFilePath should be false", bRet);
       bRet=tfsManager.rmFile(appId, userId, "/text1");
	}
	@Test
	public void test_06_mvFile_wrong_destFilePath_3()
	{
	   boolean bRet;
	   log.info( "test_06_mvFile_wrong_destFilePath_3" );
	   bRet=tfsManager.createFile(appId, userId, "/textmvFile6src");
	   bRet=tfsManager.mvFile(appId, userId, "/textmvFile6src","///textmvFile6tar///");
       Assert.assertTrue("mvFile with wrong_3 destFilePath be true", bRet);
       bRet=tfsManager.rmFile(appId, userId, "/textmvFile6tar");
	}
	@Test
	public void test_07_mvFile_leap_Filename()
	{
	   boolean bRet;
	   log.info( "test_07_mvFile_leap_Filename" );
	   bRet=tfsManager.createFile(appId, userId, "/text1");
	   bRet=tfsManager.mvFile(appId, userId, "/text1","/text2/text");
       Assert.assertFalse("mvFile with leap destFilePath should be false", bRet);
       bRet=tfsManager.rmFile(appId, userId, "/text1");
	}
	@Test
	public void test_08_mvFile_exist_Filename()
	{
	   boolean bRet;
	   log.info( "test_08_mvFile_exist_Filename" );
	   bRet=tfsManager.createFile(appId, userId, "/text1");
	   bRet=tfsManager.createFile(appId, userId, "/text2");
	   bRet=tfsManager.mvFile(appId, userId, "/text1","/text2");
       Assert.assertFalse("mvFile with exist destFilePath should be false", bRet);
       bRet=tfsManager.rmFile(appId, userId, "/text1");
       bRet=tfsManager.rmFile(appId, userId, "/text2");
	}
	@Test
	public void test_09_mvFile_null_srcFilePath()
	{
	   boolean bRet;
	   log.info( "test_09_mvFile_null_srcFilePath" );
	   bRet=tfsManager.mvFile(appId, userId, null,"/text2");
       Assert.assertFalse("mvFile with null srcFilePath should be false", bRet);
    }
	@Test
	public void test_10_mvFile_empty_srcFilePath()
	{
	   boolean bRet;
	   log.info( "test_10_mvFile_empty_srcFilePath" );
	   bRet=tfsManager.mvFile(appId, userId, "","/text2");
       Assert.assertFalse("mvFile with empty srcFilePath should be false", bRet);
    }
	@Test
	public void test_11_mvFile_wrong_srcFilePath_1()
	{
	   boolean bRet;
	   log.info( "test_11_mvFile_wrong_srcFilePath_1" );
	   bRet=tfsManager.mvFile(appId, userId, "/","/text2");
       Assert.assertFalse("mvFile with wrong-1 srcFilePath should be false", bRet);
    }
	@Test
	public void test_12_mvFile_wrong_srcFilePath_2()
	{
	   boolean bRet;
	   log.info( "test_12_mvFile_wrong_srcFilePath_2" );
	   bRet=tfsManager.createFile(appId, userId, "/text1");
	   bRet=tfsManager.mvFile(appId, userId, "text1","/text2");
       Assert.assertFalse("mvFile with wrong-2 srcFilePath should be false", bRet);
       tfsManager.rmFile(appId, userId, "/text1");
    }
	@Test
	public void test_13_mvFile_with_same_name()
	{
	   boolean bRet;
	   log.info( "test_13_mvFile_with_same_name" );
	   bRet=tfsManager.createFile(appId, userId, "/text1");
	   bRet=tfsManager.mvFile(appId, userId, "/text1","/text1");
       Assert.assertFalse("mvFile with the same name should be false", bRet);
       tfsManager.rmFile(appId, userId, "/text1");
    }
	@Test
	public void test_14_mvFile_dif_uid()
	{
	   boolean bRet;
	   log.info( "test_14_mvFile_dif_uid" );
	   bRet=tfsManager.mvFile(appId, userId, "/text1", "/text2");
       Assert.assertFalse("mvFile with different uid should be false", bRet);
	}

}
