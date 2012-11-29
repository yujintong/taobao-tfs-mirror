package com.taobao.common.tfs.testcase.Interface.normal.rc;

import java.io.IOException;

import junit.framework.Assert;

import org.junit.Test;


import com.taobao.common.tfs.packet.FileInfo;
import com.taobao.common.tfs.packet.UnlinkFileMessage;
import com.taobao.common.tfs.testcase.rcTfsBaseCase;
import com.taobao.common.tfs.utility.FileUtility;


public class RcTfsManager_13_unlinkFile extends rcTfsBaseCase 
{
	@Test  
    public  void  test_01_saveFile_then_unlinkFile()
	{
		log.info(new Throwable().getStackTrace()[0].getMethodName());
		String Ret=null;
		Ret=tfsManager.saveFile( resourcesPath+"100K.jpg",null,null,false);
		Assert.assertNotNull(Ret);
		System.out.println("The tfs file name is "+ Ret);
		boolean Bret;
		Bret=tfsManager.unlinkFile(Ret, null);
		Assert.assertTrue(Bret);
	}
	
	@Test
    public  void  test_02_saveFile_simpleName_then_unlinkFile()
	{
		log.info(new Throwable().getStackTrace()[0].getMethodName());
		String Ret=null;
		Ret=tfsManager.saveFile( resourcesPath+"2M.jpg",null,null,true);
		Assert.assertNotNull(Ret);
		System.out.println("The tfs file name is "+ Ret);
		boolean Bret;
		Bret=tfsManager.unlinkFile(Ret, null);
		Assert.assertTrue(Bret);
	}
	
	@Test
    public  void  test_03_saveFile_byte_then_unlinkFile() throws IOException
	{
		log.info(new Throwable().getStackTrace()[0].getMethodName());
		String Ret=null;
		byte data[]=null;
		data=FileUtility.getByte(resourcesPath+"100K.jpg");
		Ret=tfsManager.saveFile(null,null,data,0,100*(1<<10),false);
		Assert.assertNotNull(Ret);
		System.out.println("The tfs file name is "+ Ret);
		
		boolean Bret;
		Bret=tfsManager.unlinkFile(Ret, null);
		Assert.assertTrue(Bret);
	}
	
	@Test
    public  void  test_04_saveFile_byte_simpleName_then_unlinkFile() throws IOException
	{
		log.info(new Throwable().getStackTrace()[0].getMethodName());
		String Ret=null;
		byte data[]=null;
		data=FileUtility.getByte(resourcesPath+"100K.jpg");
		Ret=tfsManager.saveFile(null,null,data,0,100*(1<<10),true);
		Assert.assertNotNull(Ret);
		System.out.println("The tfs file name is "+ Ret);
		
		boolean Bret;
		Bret=tfsManager.unlinkFile(Ret, null);
		Assert.assertTrue(Bret);
	}
	
	@Test
    public  void  test_05_saveLargeFile_then_unlinkFile()
	{
		log.info(new Throwable().getStackTrace()[0].getMethodName());
		String Ret=null;
		Ret=tfsManager.saveLargeFile( resourcesPath+"5M.jpg",null,null);
		Assert.assertNotNull(Ret);
		System.out.println("The tfs file name is "+ Ret);
		
		boolean Bret;
		Bret=tfsManager.unlinkFile(Ret, null);
		Assert.assertTrue(Bret);
	}
	
	@Test
    public  void  test_06_saveLargeFile_byte_then_unlinkFile() throws IOException
	{
		log.info(new Throwable().getStackTrace()[0].getMethodName());
		String Ret=null;
		byte data[]=null;
		data=FileUtility.getByte(resourcesPath+"5M.jpg");
		Ret=tfsManager.saveLargeFile(null,null,data,0,data.length,key);
		Assert.assertNotNull(Ret);
		System.out.println("The tfs file name is "+ Ret);
		
		FileInfo info=new FileInfo();
		info=tfsManager.statFile(Ret, null);
		System.out.println(info);
		
		boolean Bret;
		Bret=tfsManager.unlinkFile(Ret, null);
		
		info=tfsManager.statFile(Ret, null);
		System.out.println(info);
		
		Assert.assertTrue(Bret);
	}
	
	@Test
    public  void  test_07_unlinkFile_wrong_tfsFileName()
	{
		log.info(new Throwable().getStackTrace()[0].getMethodName());
		boolean Bret;
		Bret=tfsManager.unlinkFile("sahjksajkdfhskl", null);
		Assert.assertFalse(Bret);
	}
	
	@Test
    public  void  test_08_unlinkFile_null_tfsFileName()
	{
		log.info(new Throwable().getStackTrace()[0].getMethodName());
		boolean Bret;
		Bret=tfsManager.unlinkFile(null, null);
		Assert.assertFalse(Bret);
	}
	
	@Test
    public  void  test_09_unlinkFile_empty_tfsFileName()
	{
		log.info(new Throwable().getStackTrace()[0].getMethodName());
		boolean Bret;
		Bret=tfsManager.unlinkFile("", null);
		Assert.assertFalse(Bret);
	}
	
	@Test
    public  void  test_10_openReadFile_then_unlinkFile() throws IOException
	{
		log.info(new Throwable().getStackTrace()[0].getMethodName());
	
		String Ret=null;
		Ret=tfsManager.saveFile( resourcesPath+"100K.jpg",null,null,false);
		Assert.assertNotNull(Ret);
		System.out.println("The tfs file name is "+ Ret);
		
		int fd=-1;
		fd=tfsManager.openReadFile(Ret, null);
		Assert.assertTrue(fd>0);
		
		boolean Bret;
		Bret=tfsManager.unlinkFile(Ret, null);
		Assert.assertTrue(Bret);
		
	}
	
	@Test
    public  void  test_11_openWriteFile_then_unlinkFile() throws IOException
	{
		log.info(new Throwable().getStackTrace()[0].getMethodName());
	
		String Ret=null;
		
		int fd=-1;
		fd=tfsManager.openWriteFile(Ret, null,key);
		Assert.assertTrue(fd>0);
		
		int Wret;
		byte []data=null;
		data=FileUtility.getByte(resourcesPath+"5M.jpg");
		Wret=tfsManager.writeFile(fd, data, 0, data.length);
		Assert.assertTrue(Wret>0);
		
		Ret=tfsManager.closeFile(fd);
		System.out.println(Ret);
		
		boolean Bret;
		Bret=tfsManager.unlinkFile(Ret, null);
		Assert.assertTrue(Bret);	
	}
	
	@Test
    public  void  test_12_saveFile_then_unlinkFile_wrong_suffix()
	{
		log.info(new Throwable().getStackTrace()[0].getMethodName());
		String Ret=null;
		Ret=tfsManager.saveFile( resourcesPath+"100K.jpg",null,null,false);
		Assert.assertNotNull(Ret);
		System.out.println("The tfs file name is "+ Ret);
		
		boolean Bret;
		Bret=tfsManager.unlinkFile(Ret, ".jpg");
		Assert.assertFalse(Bret);
	}
	
	@Test
    public  void  test_13_saveFile_then_unlinkFile_empty_suffix()
	{
		log.info(new Throwable().getStackTrace()[0].getMethodName());
		String Ret=null;
		Ret=tfsManager.saveFile( resourcesPath+"100K.jpg",null,null,false);
		Assert.assertNotNull(Ret);
		System.out.println("The tfs file name is "+ Ret);
		
		boolean Bret;
		Bret=tfsManager.unlinkFile(Ret, "");
		Assert.assertTrue(Bret);
	}
	
	@Test
    public  void  test_14_saveFile_then_unlinkFile_double()
	{
		log.info(new Throwable().getStackTrace()[0].getMethodName());
		String Ret=null;
		Ret=tfsManager.saveFile( resourcesPath+"100K.jpg",null,null,false);
		Assert.assertNotNull(Ret);
		System.out.println("The tfs file name is "+ Ret);
		
		boolean Bret;
		Bret=tfsManager.unlinkFile(Ret, null);
		Assert.assertTrue(Bret);
		
		Bret=tfsManager.unlinkFile(Ret, null);
		Assert.assertFalse(Bret);
	}
	
	@Test
    public  void  test_15_saveFile_then_linkFile()
	{
		log.info(new Throwable().getStackTrace()[0].getMethodName());
		String Ret=null;
		Ret=tfsManager.saveFile( resourcesPath+"100K.jpg",null,null,false);
		Assert.assertNotNull(Ret);
		System.out.println("The tfs file name is "+ Ret);
		
		boolean Bret;
		Bret=tfsManager.unlinkFile(Ret, null,UnlinkFileMessage.UNDELETE);
		
		
		Assert.assertFalse(Bret);
	}
	
	@Test
    public  void  test_16_saveFile_then_unlinkFile_linkFile()
	{
		log.info(new Throwable().getStackTrace()[0].getMethodName());
		String Ret=null;
		Ret=tfsManager.saveFile( resourcesPath+"100K.jpg",null,null,false);
		Assert.assertNotNull(Ret);
		System.out.println("The tfs file name is "+ Ret);
		
		boolean Bret;
		Bret=tfsManager.unlinkFile(Ret, null);
		Assert.assertTrue(Bret);
		
		Bret=tfsManager.unlinkFile(Ret, null,UnlinkFileMessage.UNDELETE);
		Assert.assertTrue(Bret);
	}
	
	@Test
    public  void  test_17_saveFile_then_unlinkFile_wrong_suffix()
	{
		log.info(new Throwable().getStackTrace()[0].getMethodName());
		String Ret=null;
		Ret=tfsManager.saveFile( resourcesPath+"100K.jpg",null,null,false);
		Assert.assertNotNull(Ret);
		System.out.println("The tfs file name is "+ Ret);
		
		boolean Bret;
		Bret=tfsManager.unlinkFile(Ret, ".jpg");
		Assert.assertFalse(Bret);
	}
	
	@Test
    public  void  test_18_saveFile_with_suffix_then_unlinkFile_with_suffix()
	{
		log.info(new Throwable().getStackTrace()[0].getMethodName());
		String Ret=null;
		Ret=tfsManager.saveFile( resourcesPath+"100K.jpg",null,".jpg",false);
		Assert.assertNotNull(Ret);
		System.out.println("The tfs file name is "+ Ret);
		
		boolean Bret;
		Bret=tfsManager.unlinkFile(Ret, ".jpg");
		Assert.assertTrue(Bret);
	}
}