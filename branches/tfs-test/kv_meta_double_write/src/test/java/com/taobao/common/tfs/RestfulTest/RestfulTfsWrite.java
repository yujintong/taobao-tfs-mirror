package com.taobao.common.tfs.RestfulTest;



import java.io.IOException;

import org.junit.Test;
import org.junit.Ignore;

import junit.framework.Assert;

import com.taobao.common.tfs.RestfulTfsBaseCase;
import com.taobao.common.tfs.TfsConstant;



public class RestfulTfsWrite extends RestfulTfsBaseCase 
{
	@Test
    public void test_01_write_right() throws IOException 
	{
	   log.info(new Throwable().getStackTrace()[0].getMethodName());
	   String File_name = "textwrite";
	   tfsManager.createFile(appId, userId, "/"+File_name); 
	   Assert.assertTrue(HeadObject(buecket_name,File_name,0));
	   
	   int localCrc = getCrc(resourcesPath+"10K.jpg"); 
	   byte data[]=null;
	   data=getByte(resourcesPath+"10K.jpg");
	   Assert.assertNotNull(data);
	   long len = data.length;
	   long  offset=0;
	   long dataOffset=0;
	   long  Ret;
	   Ret=tfsManager.write(appId, userId, "/"+File_name, offset,data, dataOffset, len);
	   Assert.assertEquals(Ret,len);
	   Assert.assertTrue(HeadObject(buecket_name,File_name,10*(1<<10)));
	   
	   tfsManager.fetchFile(appId, userId, resourcesPath+"temp", "/"+File_name);
	   int TfsCrc = getCrc(resourcesPath+"temp");
	   Assert.assertEquals(TfsCrc,localCrc);
	   
	   GetObject(buecket_name,File_name,TempFile);
	   int kvmetaCrc = getCrc(TempFile);
	   Assert.assertEquals(kvmetaCrc,localCrc);
	   
	   tfsManager.rmFile(appId, userId, "/"+File_name);
	   Assert.assertFalse(HeadObject(buecket_name,File_name));
    }

	@Test
    public void test_02_write_more_offset() throws IOException 
	{
	   log.info(new Throwable().getStackTrace()[0].getMethodName());
	   String File_name = "textwrite";
	   tfsManager.createFile(appId, userId, "/"+File_name); 
	   int localCrc = getCrc(resourcesPath+"10K.jpg"); 
	   Assert.assertTrue(HeadObject(buecket_name,File_name,0));
	   
	   byte data[]=null;
	   data=getByte(resourcesPath+"10K.jpg");
	   long len = data.length;
	   long offset=0;
	   long dataOffset=0;
	   long  Ret;
	   Ret=tfsManager.write(appId, userId, "/"+File_name, offset,data, dataOffset, len);
	   Assert.assertEquals(Ret,10*(1<<10));
	   Assert.assertTrue(HeadObject(buecket_name,File_name,10*(1<<10)));
	   
	   tfsManager.fetchFile(appId, userId, resourcesPath+"temp", "/"+File_name);
	   int TfsCrc = getCrc(resourcesPath+"temp");
	   Assert.assertEquals(TfsCrc,localCrc);
	   tfsManager.rmFile(appId, userId, "/"+File_name);
	   Assert.assertFalse(HeadObject(buecket_name,File_name));
    }
	@Test
    public void test_03_write_n_1_offset() throws IOException 
	{
	   log.info(new Throwable().getStackTrace()[0].getMethodName());
	   String File_name = "textwrite";
	   tfsManager.createFile(appId, userId, "/"+File_name);
	   Assert.assertTrue(HeadObject(buecket_name,File_name,0));
	   
	   int localCrc = getCrc(resourcesPath+"10K.jpg"); 
	   byte data[]=null;
	   data=getByte(resourcesPath+"10K.jpg");
	   long len = 10*(1<<10);
	   long offset=-1;
	   long dataOffset=0;
	   long  Ret;
	   Ret=tfsManager.write(appId, userId, "/"+File_name, offset,data, dataOffset, len);
	   Assert.assertEquals(Ret,10*(1<<10));
	   Assert.assertTrue(HeadObject(buecket_name,File_name,10*(1<<10)));
	  
	   tfsManager.fetchFile(appId, userId, resourcesPath+"temp", "/"+File_name);
	   int TfsCrc = getCrc(resourcesPath+"temp");
	   Assert.assertEquals(TfsCrc,localCrc);
	   
	   GetObject(buecket_name,File_name,TempFile);
	   int kvmetaCrc = getCrc(TempFile);
	   Assert.assertEquals(kvmetaCrc,localCrc);
	   
	   tfsManager.rmFile(appId, userId, "/"+File_name);
	   Assert.assertFalse(HeadObject(buecket_name,File_name));
    }
	@Test
    public void test_04_write_wrong_offset() throws IOException 
	{
	   log.info(new Throwable().getStackTrace()[0].getMethodName());
	   String File_name = "textwrite";
	   tfsManager.createFile(appId, userId, "/"+File_name); 
	   Assert.assertTrue(HeadObject(buecket_name,File_name,0));
	   byte data[]=null;
	   data=getByte(resourcesPath+"10K.jpg");
	   long len = 10*(1<<10);
	   long offset=-2;
	   long dataOffset=0;
	   long  Ret;
	   Ret=tfsManager.write(appId, userId, "/"+File_name, offset,data, dataOffset, len);
	   //Assert.assertNotEquals(Ret,10*(1<<10));
	   Assert.assertFalse(Ret>=0);
	   Assert.assertTrue(HeadObject(buecket_name,File_name,0));
	   tfsManager.rmFile(appId, userId, "/"+File_name);
	   Assert.assertFalse(HeadObject(buecket_name,File_name));
    }
	@Test
    public void test_05_write_with_dataOffset() throws IOException 
	{
	   log.info(new Throwable().getStackTrace()[0].getMethodName());
	   String File_name = "textwrite";
	   tfsManager.createFile(appId, userId, "/"+File_name); 
	   Assert.assertTrue(HeadObject(buecket_name,File_name,0));
	   
	   byte data[]=null;
	   data=getByte(resourcesPath+"10K.jpg");
	   long len = 9*(1<<10);
	   long offset=0;
	   long dataOffset=1023;
	   long  Ret;
	   Ret=tfsManager.write(appId, userId, "/"+File_name, offset,data, dataOffset, len);
	   System.out.println(Ret);
	   Assert.assertEquals(Ret,9*(1<<10));
	   
	   Assert.assertTrue(HeadObject(buecket_name,File_name,9*(1<<10)));
	   tfsManager.fetchFile(appId, userId, resourcesPath+"temp", "/"+File_name);
	   tfsManager.rmFile(appId, userId, "/"+File_name);
	   Assert.assertFalse(HeadObject(buecket_name,File_name));
    }
	@Test
    public void test_06_write_more_dataOffset() throws IOException 
	{
	   log.info(new Throwable().getStackTrace()[0].getMethodName());
	   String File_name = "textwrite";
	   
	   tfsManager.createFile(appId, userId, "/"+File_name); 
	   Assert.assertTrue(HeadObject(buecket_name,File_name,0));
	   
	   byte data[]=null;
	   data=getByte(resourcesPath+"10K.jpg");
	   long len = 10*(1<<10);
	   long offset=0;
	   long dataOffset=10*(1<<10)+100;
	   long  Ret;
	   Ret=tfsManager.write(appId, userId, "/"+File_name, offset,data, dataOffset, len);
	   Assert.assertTrue(Ret<0);
	   Assert.assertTrue(HeadObject(buecket_name,File_name,0));
	   
	   tfsManager.fetchFile(appId, userId, resourcesPath+"temp", "/"+File_name);
	   //Assert.assertEquals(TfsCrc,localCrc);
	   tfsManager.rmFile(appId, userId, "/"+File_name);
	   Assert.assertFalse(HeadObject(buecket_name,File_name));
    }
	@Test
    public void test_07_write_wrong_dataOffset() throws IOException 
	{
	   log.info(new Throwable().getStackTrace()[0].getMethodName());
	   String File_name = "textwrite";
	   tfsManager.createFile(appId, userId, "/"+File_name); 
	   Assert.assertTrue(HeadObject(buecket_name,File_name,0));
	   
	   byte data[]=null;
	   data=getByte(resourcesPath+"10K.jpg");
	   long len = 10*(1<<10);
	   long offset=0;
	   long dataOffset=-1;
	   long  Ret;
	   Ret=tfsManager.write(appId, userId, "/"+File_name, offset,data, dataOffset, len);
	   Assert.assertFalse(Ret>=0);
	   Assert.assertTrue(HeadObject(buecket_name,File_name,0));
	   
	   tfsManager.fetchFile(appId, userId, resourcesPath+"temp", "/"+File_name);
	   tfsManager.rmFile(appId, userId, "/"+File_name);
	   Assert.assertFalse(HeadObject(buecket_name,File_name));
    }
	@Test
    public void test_08_write_null_data() throws IOException 
	{
	   log.info(new Throwable().getStackTrace()[0].getMethodName());
	   String File_name = "textwrite";
	   tfsManager.createFile(appId, userId, "/"+File_name); 
	   Assert.assertTrue(HeadObject(buecket_name,File_name,0));
	   
	   byte data[]=null;
	   long len = 0;
	   long offset=0;
	   long dataOffset=0;
	   long  Ret;
	   Ret=tfsManager.write(appId, userId, "/"+File_name, offset,data, dataOffset, len);
	   Assert.assertTrue(Ret<0);
	   Assert.assertTrue(HeadObject(buecket_name,File_name,0));
	   
	   tfsManager.fetchFile(appId, userId, resourcesPath+"temp", "/"+File_name);
	   //Assert.assertEquals(TfsCrc,localCrc);
	   tfsManager.rmFile(appId, userId, "/"+File_name);
	   Assert.assertFalse(HeadObject(buecket_name,File_name));
    }
	@Test
    public void test_09_write_empty_data() 
	{
	   log.info(new Throwable().getStackTrace()[0].getMethodName());
	   String File_name = "textwrite";
	   tfsManager.createFile(appId, userId, "/"+File_name); 
	   Assert.assertTrue(HeadObject(buecket_name,File_name,0));
	   
	   byte data[]=null;
	   long len = 0;
	   long offset=0;
	   long dataOffset=0;
	   long  Ret;
	   Ret=tfsManager.write(appId, userId, "/"+File_name, offset,data, dataOffset, len);
	   Assert.assertTrue(Ret<0);
	   Assert.assertTrue(HeadObject(buecket_name,File_name,0));
	   
	   tfsManager.fetchFile(appId, userId, resourcesPath+"temp", "/"+File_name);
	   //Assert.assertEquals(TfsCrc,localCrc);
	   tfsManager.rmFile(appId, userId, "/"+File_name);
	   Assert.assertFalse(HeadObject(buecket_name,File_name));
    }
	@Test
    public void test_10_write_null_filename() throws IOException 
	{
	   log.info(new Throwable().getStackTrace()[0].getMethodName());
	   String File_name = "textwrite";
	   tfsManager.createFile(appId, userId, "/"+File_name); 
	   Assert.assertTrue(HeadObject(buecket_name,File_name,0));
	   
	   byte data[]=null;
	   data=getByte(resourcesPath+"10K.jpg");
	   long len = 10*(1<<10);
	   long offset=0;
	   long dataOffset=0;
	   long  Ret;
	   Ret=tfsManager.write(appId, userId, null, offset,data, dataOffset, len);
	   Assert.assertFalse(Ret>=0);
	   tfsManager.fetchFile(appId, userId, resourcesPath+"temp", "/"+File_name);
	   Assert.assertTrue(HeadObject(buecket_name,File_name,0));
	   
	   //Assert.assertEquals(TfsCrc,localCrc);
	   tfsManager.rmFile(appId, userId, "/"+File_name);
	   Assert.assertFalse(HeadObject(buecket_name,File_name));
    }
	@Test
    public void test_11_write_empty_filename() throws IOException 
	{
	   log.info(new Throwable().getStackTrace()[0].getMethodName());
	   String File_name = "textwrite";
	   tfsManager.createFile(appId, userId, "/"+File_name); 
	   Assert.assertTrue(HeadObject(buecket_name,File_name,0));
	   
	   byte data[]=null;
	   data=getByte(resourcesPath+"10K.jpg");
	   long len = 10*(1<<10);
	   long offset=0;
	   long dataOffset=0;
	   long  Ret;
	   Ret=tfsManager.write(appId, userId, "", offset,data, dataOffset, len);
	   Assert.assertFalse(Ret>=0);
	   tfsManager.fetchFile(appId, userId, resourcesPath+"temp", "/"+File_name);
	   Assert.assertTrue(HeadObject(buecket_name,File_name,0));
	   
	   //Assert.assertEquals(TfsCrc,localCrc);
	   tfsManager.rmFile(appId, userId, "/"+File_name);
	   Assert.assertFalse(HeadObject(buecket_name,File_name));
    }
	@Test
    public void test_12_write_wrong_filename_1() throws IOException 
	{
	   log.info(new Throwable().getStackTrace()[0].getMethodName());
	   String File_name = "textwrite";
	   tfsManager.createFile(appId, userId, "/"+File_name);
	   Assert.assertTrue(HeadObject(buecket_name,File_name,0));
	   
	   int localCrc = getCrc(resourcesPath+"10K.jpg"); 
	   byte data[]=null;
	   data=getByte(resourcesPath+"10K.jpg");
	   long len = 10*(1<<10);
	   long offset=0;
	   long dataOffset=0;
	   long  Ret;
	   Ret=tfsManager.write(appId, userId, "///textwrite///", offset,data, dataOffset, len);
	   Assert.assertEquals(Ret,10*(1<<10));
	   tfsManager.fetchFile(appId, userId, resourcesPath+"temp", "/"+File_name);
	   Assert.assertTrue(HeadObject(buecket_name,File_name,10*(1<<10)));
	   
	   int TfsCrc = getCrc(resourcesPath+"10K.jpg");
	   Assert.assertEquals(TfsCrc,localCrc);
	   
	   GetObject(buecket_name,File_name,TempFile);
	   int kvmetaCrc = getCrc(TempFile);
	   Assert.assertEquals(kvmetaCrc,localCrc);
	   
	   tfsManager.rmFile(appId, userId, "/"+File_name);
	   Assert.assertFalse(HeadObject(buecket_name,File_name));
    }
	@Test
    public void test_13_write_wrong_filename_2() throws IOException 
	{
	   log.info(new Throwable().getStackTrace()[0].getMethodName());
	   String File_name = "textwrite";
	   tfsManager.createFile(appId, userId, "/"+File_name); 
	   Assert.assertTrue(HeadObject(buecket_name,File_name,0));
	   
	   byte data[]=null;
	   data=getByte(resourcesPath+"10K.jpg");
	   long len = 10*(1<<10);
	   long offset=0;
	   long dataOffset=0;
	   long  Ret;
	   Ret=tfsManager.write(appId, userId, "textwrite", offset,data, dataOffset, len);
	   Assert.assertFalse(Ret>=0);
	   tfsManager.fetchFile(appId, userId, resourcesPath+"temp", "/"+File_name);
	   //Assert.assertEquals(TfsCrc,localCrc);
	   Assert.assertTrue(HeadObject(buecket_name,File_name,0));
	   
	   tfsManager.rmFile(appId, userId, "/"+File_name);
	   Assert.assertFalse(HeadObject(buecket_name,File_name));
    }
	@Test
    public void test_14_write_wrong_filename_3() throws IOException 
	{
	   log.info(new Throwable().getStackTrace()[0].getMethodName());
	   String File_name = "textwrite";
	   tfsManager.createFile(appId, userId, "/"+File_name); 
	   byte data[]=null;
	   data=getByte(resourcesPath+"10K.jpg");
	   long len = 10*(1<<10);
	   long offset=0;
	   long dataOffset=0;
	   long  Ret;
	   Ret=tfsManager.write(appId, userId, "/", offset,data, dataOffset, len);
	   //System.out.println("%%%%%%%%%%%%%%%%%%%%%%%%"+Ret);
	   Assert.assertEquals(Ret,TfsConstant.EXIT_INVALID_ARGU_ERROR);
	   tfsManager.fetchFile(appId, userId, resourcesPath+"temp", "/"+File_name);
	   //Assert.assertEquals(TfsCrc,localCrc);
	   tfsManager.rmFile(appId, userId, "/"+File_name);
    }
	@Test
    public void test_15_write_Dir() throws IOException 
	{
	   log.info(new Throwable().getStackTrace()[0].getMethodName());
	   String File_name = "textwrite";
	   tfsManager.createDir(appId, userId, "/"+File_name); 
	   byte data[]=null;
	   data=getByte(resourcesPath+"10K.jpg");
	   long len = 10*(1<<10);
	   long offset=0;
	   long dataOffset=0;
	   long  Ret;
	   Ret=tfsManager.write(appId, userId, "/"+File_name, offset,data, dataOffset, len);
	   Assert.assertFalse(Ret>0);
	   tfsManager.rmDir(appId, userId, "/"+File_name);
    }
	@Test
    public void test_16_write_many_times() throws IOException 
	{
	   log.info(new Throwable().getStackTrace()[0].getMethodName());
	   String File_name = "textwrite";
	   tfsManager.createFile(appId, userId, "/"+File_name);
	   Assert.assertTrue(HeadObject(buecket_name,File_name,0));
	   
	   int localCrc = getCrc(resourcesPath+"10K.jpg"); 
	   byte data[]=null;
	   data=getByte(resourcesPath+"10K.jpg");
	   long len = 10*(1<<10);
	   long offset=0;
	   long dataOffset=0;
	   long  Ret;
	   Ret=tfsManager.write(appId, userId, "/"+File_name, offset,data, dataOffset, len);
	   Assert.assertEquals(Ret,10*(1<<10));
	   Assert.assertTrue(HeadObject(buecket_name,File_name,10*(1<<10)));
	   
	   Ret=tfsManager.write(appId, userId, "/"+File_name, offset,data, dataOffset, len);
	   System.out.println(Ret);
	   Assert.assertTrue(Ret<0);
	   Assert.assertTrue(HeadObject(buecket_name,File_name,10*(1<<10)));
	   
	   tfsManager.fetchFile(appId, userId, resourcesPath+"temp", "/"+File_name);
	   int TfsCrc = getCrc(resourcesPath+"10K.jpg");
	   Assert.assertEquals(TfsCrc,localCrc);
	   tfsManager.rmFile(appId, userId, "/"+File_name);
	   Assert.assertFalse(HeadObject(buecket_name,File_name));
    }
	@Test
    public void test_17_write_not_exist() throws IOException 
	{
	   log.info(new Throwable().getStackTrace()[0].getMethodName());
	   byte data[]=null;
	   data=getByte(resourcesPath+"10K.jpg");
	   long len = 10*(1<<10);
	   long offset=0;
	   long dataOffset=0;
	   long  Ret;
	   Ret=tfsManager.write(appId, userId, "/textwrite", offset,data, dataOffset, len);
	   Assert.assertFalse(Ret==0);
    }
	@Test
    public void test_18_write_large() throws IOException 
	{
	   log.info(new Throwable().getStackTrace()[0].getMethodName());
	   String File_name = "textwrite";
	   tfsManager.createFile(appId, userId, "/"+File_name); 
	   Assert.assertTrue(HeadObject(buecket_name,File_name,0));
	   
	   int localCrc = getCrc(resourcesPath+"10M.jpg"); 
	   byte data[]=null;
	   data=getByte(resourcesPath+"10M.jpg");
	   long len = 10*(1<<20);
	   long offset=0;
	   long dataOffset=0;
	   long  Ret;
	   Ret=tfsManager.write(appId, userId, "/"+File_name, offset,data, dataOffset, len);
	   Assert.assertEquals(Ret,10*(1<<20));
	   tfsManager.fetchFile(appId, userId, resourcesPath+"temp", "/"+File_name);
	   int TfsCrc = getCrc(resourcesPath+"10M.jpg");
	   Assert.assertEquals(TfsCrc,localCrc);
	   Assert.assertTrue(HeadObject(buecket_name,File_name,10*(1<<20)));
	   
	   GetObject(buecket_name,File_name,TempFile);
	   int kvmetaCrc = getCrc(TempFile);
	   Assert.assertEquals(kvmetaCrc,localCrc);
	   
	   tfsManager.rmFile(appId, userId, "/"+File_name);
	   Assert.assertFalse(HeadObject(buecket_name,File_name));
    }
	@Test
    public void test_19_write_many_times_parts() throws IOException 
	{
	   log.info(new Throwable().getStackTrace()[0].getMethodName());
	   String File_name = "textwrite";
	   tfsManager.createFile(appId, userId, "/"+File_name);
	   Assert.assertTrue(HeadObject(buecket_name,File_name,0));
	   
       byte data[]=null;
       data=getByte(resourcesPath+"10K.jpg");
	   long len = 10*(1<<10);
	   long offset=0;
	   long dataOffset=0;
	   long  Ret;
	   Ret=tfsManager.write(appId, userId, "/"+File_name, offset,data, dataOffset, len);
	   Assert.assertEquals(Ret,10*(1<<10));
	   Assert.assertTrue(HeadObject(buecket_name,File_name,10*(1<<10)));
	   
	   len = 10*(1<<10);
	   offset=10*(1<<10);
	   dataOffset=0;
	   Ret=tfsManager.write(appId, userId, "/"+File_name, offset,data, dataOffset, len);
	   Assert.assertEquals(Ret,10*(1<<10));
	   Assert.assertTrue(HeadObject(buecket_name,File_name,20*(1<<10)));
	   
	   len = 10*(1<<10);
	   offset=20*(1<<10)+1;
	   dataOffset=0;
	   Ret=tfsManager.write(appId, userId, "/"+File_name, offset,data, dataOffset, len);
	   Assert.assertEquals(Ret,10*(1<<10));
	   Assert.assertTrue(HeadObject(buecket_name,File_name,30*(1<<10)+1));
	   
	   data=getByte(resourcesPath+"1B.jpg");
	   len = 1;
	   offset=20*(1<<10);
	   dataOffset=0;
	   Ret=tfsManager.write(appId, userId, "/"+File_name, offset,data, dataOffset, len);
	   Assert.assertEquals(Ret,1);
	   Assert.assertTrue(HeadObject(buecket_name,File_name,30*(1<<10)+1));
	   
	   data=getByte(resourcesPath+"1B.jpg");
	   len = 1;
	   offset=20*(1<<10);
	   dataOffset=0;
	   Ret=tfsManager.write(appId, userId, "/"+File_name, offset,data, dataOffset, len);
	   Assert.assertTrue(Ret<0);
	   tfsManager.fetchFile(appId, userId, resourcesPath+"temp", "/"+File_name);
	   Assert.assertTrue(HeadObject(buecket_name,File_name,30*(1<<10)+1));
	   
	   tfsManager.rmFile(appId, userId, "/"+File_name);
	   Assert.assertFalse(HeadObject(buecket_name,File_name));
    }
	 @Test
	 public void test_20_write_large_many_times_parts() throws IOException 
		{
		   log.info(new Throwable().getStackTrace()[0].getMethodName());
		   String File_name = "textwrite";
		   tfsManager.createFile(appId, userId, "/"+File_name); 
		   Assert.assertTrue(HeadObject(buecket_name,File_name,0));
		   
	       byte data[]=null;
	       data=getByte(resourcesPath+"3M.jpg");
	       System.out.println(data.length);
		   long len = 3*(1<<20);
		   long offset=0;
		   long dataOffset=0;
		   long  Ret;
		   Ret=tfsManager.write(appId, userId, "/"+File_name, offset,data, dataOffset, len);
		   Assert.assertEquals(Ret,3*(1<<20));
		   Assert.assertTrue(HeadObject(buecket_name,File_name,3*(1<<20)));
		   
		   len = 3*(1<<20);
		   offset=3*(1<<20);
		   dataOffset=0;
		   Ret=tfsManager.write(appId, userId, "/"+File_name, offset,data, dataOffset, len);
		   Assert.assertEquals(Ret,3*(1<<20));
		   Assert.assertTrue(HeadObject(buecket_name,File_name,6*(1<<20)));
		   
		   len = 3*(1<<20);
		   offset=6*(1<<20)+1;
		   dataOffset=0;
		   Ret=tfsManager.write(appId, userId, "/"+File_name, offset,data, dataOffset, len);
		   Assert.assertEquals(Ret,3*(1<<20));
		   Assert.assertTrue(HeadObject(buecket_name,File_name,9*(1<<20)+1));
		   
		   data=getByte(resourcesPath+"1B.jpg");
		   Assert.assertNotNull(data);
		   len = 1;
		   offset=6*(1<<20);
		   dataOffset=0;
		   Ret=tfsManager.write(appId, userId, "/"+File_name, offset,data, dataOffset, len);
		   System.out.println("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"+Ret);
		   Assert.assertEquals(Ret,1);
		   Assert.assertTrue(HeadObject(buecket_name,File_name,9*(1<<20)+1));
		   
		   data=getByte(resourcesPath+"1B.jpg");
		   len = 1;
		   offset=3*(1<<20);
		   dataOffset=0;
		   Ret=tfsManager.write(appId, userId, "/"+File_name, offset,data, dataOffset, len);
		   Assert.assertTrue(Ret<0);
		   tfsManager.fetchFile(appId, userId, resourcesPath+"temp", "/"+File_name);
		   Assert.assertTrue(HeadObject(buecket_name,File_name,9*(1<<20)+1));
		   
		   tfsManager.rmFile(appId, userId, "/"+File_name);
		   Assert.assertFalse(HeadObject(buecket_name,File_name));
	    }
	 @Test
	 public void test_21_write_many_times_parts_com() throws IOException 
		{
		   log.info(new Throwable().getStackTrace()[0].getMethodName());
		   String File_name = "textwrite";
		   tfsManager.createFile(appId, userId, "/"+File_name); 
		   Assert.assertTrue(HeadObject(buecket_name,File_name,0));
		   
	       byte data[]=null;
	       data=getByte(resourcesPath+"10K.jpg");
		   long len = 10*(1<<10);
		   long offset=0;
		   long dataOffset=0;
		   long  Ret;
		   Ret=tfsManager.write(appId, userId, "/"+File_name, offset,data, dataOffset, len);
		   Assert.assertEquals(Ret,10*(1<<10));
		   Assert.assertTrue(HeadObject(buecket_name,File_name,10*(1<<10)));
		   
		   data=getByte(resourcesPath+"2M.jpg");
		   len = 2*(1<<20);
		   offset=20*(1<<10);
		   dataOffset=0;
		   Ret=tfsManager.write(appId, userId, "/"+File_name, offset,data, dataOffset, len);
		   System.out.println("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"+Ret);
		   Assert.assertEquals(Ret,2*(1<<20));
		   Assert.assertTrue(HeadObject(buecket_name,File_name,20*(1<<10)+2*(1<<20)));
		   
		   data=getByte(resourcesPath+"3M.jpg");
		   len = 3*(1<<20);
		   offset=8*(1<<20)+20*(1<<10);
		   dataOffset=0;
		   Ret=tfsManager.write(appId, userId, "/"+File_name, offset,data, dataOffset, len);
		   Assert.assertEquals(Ret,3*(1<<20));
		   Assert.assertTrue(HeadObject(buecket_name,File_name,20*(1<<10)+11*(1<<20)));
		   
		   data=getByte(resourcesPath+"10K.jpg");
		   len = 10*(1<<10);
		   offset=10*(1<<10);
		   dataOffset=0;
		   Ret=tfsManager.write(appId, userId, "/"+File_name, offset,data, dataOffset, len);
		   Assert.assertEquals(Ret,10*(1<<10));
		   Assert.assertTrue(HeadObject(buecket_name,File_name,20*(1<<10)+11*(1<<20)));
		   
		   data=getByte(resourcesPath+"10K.jpg");
		   len = 10*(1<<10);
		   offset=0;
		   dataOffset=0;
		   Ret=tfsManager.write(appId, userId, "/"+File_name, offset,data, dataOffset, len);
		   System.out.println("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"+Ret);
		   Assert.assertTrue(Ret<0);
		   Assert.assertTrue(HeadObject(buecket_name,File_name,20*(1<<10)+11*(1<<20)));
		   
		   data=getByte(resourcesPath+"3M.jpg");
		   len = 3*(1<<20);
		   offset=2*(1<<20)+20*(1<<10);
		   dataOffset=0;
		   Ret=tfsManager.write(appId, userId, "/"+File_name, offset,data, dataOffset, len);
		   System.out.println("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"+Ret);
		   Assert.assertEquals(Ret,3*(1<<20));
		   tfsManager.fetchFile(appId, userId, resourcesPath+"temp", "/"+File_name);
		   Assert.assertTrue(HeadObject(buecket_name,File_name,20*(1<<10)+11*(1<<20)));
		   
		   tfsManager.rmFile(appId, userId, "/"+File_name);
		   Assert.assertFalse(HeadObject(buecket_name,File_name));
	    }
	 @Test
	 public void write() throws IOException 
     {
		   log.info("write");
		   String File_name = "textwrite";
		   tfsManager.createFile(appId, userId, "/"+File_name); 
		   Assert.assertTrue(HeadObject(buecket_name,File_name,0));
		   
	       byte data[]=null;
	       data=getByte(resourcesPath+"3M.jpg");
		   long len = 3*(1<<20);
		   long offset=0;
		   long dataOffset=0;
		   long  Ret;
		   boolean bRet;
		   Ret=tfsManager.write(appId, userId, "/"+File_name, offset,data, dataOffset, len);
		   Assert.assertEquals(Ret,3*(1<<20));
		   Assert.assertTrue(HeadObject(buecket_name,File_name,3*(1<<20)));
		   
		   len = 3*(1<<20);
		   offset=3*(1<<20);
		   dataOffset=0;
		   Ret=tfsManager.write(appId, userId, "/"+File_name, offset,data, dataOffset, len);
		   Assert.assertEquals(Ret,3*(1<<20));
		   Assert.assertTrue(HeadObject(buecket_name,File_name,6*(1<<20)));
		   
		   len = 3*(1<<20);
		   offset=6*(1<<20);
		   dataOffset=0;
		   Ret=tfsManager.write(appId, userId, "/"+File_name, offset,data, dataOffset, len);
		   Assert.assertEquals(Ret,3*(1<<20));
		   
		   bRet=tfsManager.fetchFile(appId, userId, resourcesPath+"temp","/"+File_name);
		   Assert.assertTrue("Fetch File right path should be true", bRet);
		   Assert.assertTrue(HeadObject(buecket_name,File_name,9*(1<<20)));
		   
		   tfsManager.rmFile(appId, userId, "/"+File_name);
		   Assert.assertFalse(HeadObject(buecket_name,File_name));
	    }
}