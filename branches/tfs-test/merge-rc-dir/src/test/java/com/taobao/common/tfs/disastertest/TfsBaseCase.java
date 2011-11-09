/**
 * 
 */
package com.taobao.common.tfs.disastertest;

import java.util.ArrayList;
import java.util.List;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map.Entry;
import java.util.Date;

import com.taobao.gaia.AppCluster;
import com.taobao.gaia.AppGrid;
import com.taobao.gaia.AppServer;
import com.taobao.gaia.HelpConf;
import com.taobao.gaia.HelpFile;
import com.taobao.gaia.HelpHA;
import com.taobao.gaia.HelpLog;
import com.taobao.gaia.HelpProc;
import com.taobao.gaia.KillTypeEnum;

import org.apache.log4j.Logger;

public class TfsBaseCase {
    protected static Logger log = Logger.getLogger(TfsBaseCase.class.getName());

    public HelpConf conf = new HelpConf();
    public HelpHA HA = new HelpHA();
    public HelpLog Log = new HelpLog();
    public HelpProc Proc = new HelpProc();
    public HelpFile File = new HelpFile();

    String caseName;
    
    // Define
    final public int FAIL_COUNT_NOR = 0;
    final public int FAIL_COUNT_ERR = 1;

    final public static int WAIT_TIME = 30;
    final public int MIGRATE_TIME = 20;

    final public float SUCCESS_RATE = 100;
    //final public float HIGH_RATE = 99.9;
    final public float HALF_RATE = 50;
    final public float FAIL_RATE = 0;

    /**
     * 
     * @param sec
     */
    public void sleep(int sec) {
        log.debug("wait for "+sec+"s");
        try {
            Thread.sleep(sec*1000);
        }
        catch(Exception e) {
            e.printStackTrace();
        }
    }
    
    /**
     * 
     * @param strIp
     * @return
     */
    public boolean resetFailCount(AppServer appServer) {
        boolean bRet = false;
        bRet = HA.setFailCountBase(appServer.getIp(), appServer.getResName(), appServer.getMacName(), FAIL_COUNT_NOR);
        
        /* Wait */
        sleep(10);
        
        int iRet = HA.getFailCountBase(appServer.getIp(), appServer.getResName(), appServer.getMacName());
        if (iRet != 0) {
            return false;
        }
        return bRet;
    }
    
    /**
     * 
     * @param app
     * @return
     */
    public boolean setFailCount(AppServer appServer) {
        boolean bRet = false;
        bRet = HA.setFailCountBase(appServer.getIp(), appServer.getResName(), appServer.getMacName(), FAIL_COUNT_ERR);
        
        /* Wait */
        sleep(10);
        
        int iRet = HA.getFailCountBase(appServer.getIp(), appServer.getResName(), appServer.getMacName());
        if (iRet == 0) {
            return false;
        }
        return bRet;
    }
    
    /**
     * 
     * @return
     */
    public boolean resetAllFailCnt(AppServer master, AppServer slave) {
        log.info("Reset failcount start ===>");
        boolean bRet = resetFailCount(master);
        if (bRet == false) return bRet;
        
        bRet = resetFailCount(slave);
        if (bRet == false) return bRet;
        log.info("Reset failcount end ===>");
        return bRet;    
    }

    public boolean setAllFailCnt(AppServer master, AppServer slave) {
        log.info("Set failcount start ===>");
        boolean bRet = setFailCount(master);
        if (bRet == false) return bRet;
        
        bRet = setFailCount(slave);
        if (bRet == false) return bRet;
        log.info("Set failcount end ===>");
        return bRet;    
    }
    
    /**
     * 
     * @return
     */
    public boolean migrateVip(String ip, String ipAlias, String macName, String ethName) {
        boolean bRet = false;
        log.info("Migrate vip start ===>");
        bRet = HA.setVipMigrateBase(ip, ipAlias, macName);
        if (bRet ==  false) {
            return bRet;
        }
        
        /* Wait for migrate */
        sleep(MIGRATE_TIME);
        
        bRet = HA.chkVipBase(ip, ethName);
        log.info("Migrate vip end ===>");
        return bRet;    
    }

    /**mazhentong.pt
     * 
     * @param appServer
     * @return
     */
    public boolean cleanUpHA(AppServer appServer) {
        boolean bRet = false;
        bRet = HA.cleanupFailCount(appServer.getIp(), appServer.getResName(), appServer.getMacName());
        return bRet;
    }

 
    public boolean killOneServerForce(AppGrid appGrid, int clusterIdx, int serverIdx) {
        boolean bRet = false;
        AppServer cs = appGrid.getCluster(clusterIdx).getServer(serverIdx);
        bRet = cs.stop(KillTypeEnum.FORCEKILL, WAIT_TIME);
        return bRet;
    }

    public boolean cleanOneServer(AppGrid appGrid, int clusterIdx, int serverIdx) {
        boolean bRet = false;
        AppServer cs = appGrid.getCluster(clusterIdx).getServer(serverIdx);
        bRet = cs.stop(KillTypeEnum.NORMALKILL, WAIT_TIME);
        if (bRet == false)
            return bRet;

        bRet = cs.clean();
        return bRet;
    }

    public boolean startOneServer(AppGrid appGrid, int clusterIdx, int serverIdx) {
        boolean bRet = false;
        AppServer cs = appGrid.getCluster(clusterIdx).getServer(serverIdx);
        bRet = cs.start();
        return bRet;
    }

    public boolean killOneServer(AppGrid appGrid, int clusterIdx, int serverIdx) {
        boolean bRet = false;
        AppServer cs = appGrid.getCluster(clusterIdx).getServer(serverIdx);
        bRet = cs.stop(KillTypeEnum.NORMALKILL, WAIT_TIME);
        return bRet;
    }

    public boolean killOneCluster(AppGrid appGrid, int clusterIdx) {
        boolean bRet = false;
        AppCluster csCluster = appGrid.getCluster(clusterIdx);
        for(int iLoop = 0; iLoop < csCluster.getServerList().size(); iLoop ++) {
            AppServer cs = csCluster.getServer(iLoop);
            bRet = cs.stop(KillTypeEnum.NORMALKILL, WAIT_TIME);
            if (bRet == false) {
                break;
            }
        }
        return bRet;
    }

    public boolean startOneCluster(AppGrid appGrid, int clusterIdx)
    {
        boolean bRet = false;
        AppCluster csCluster = appGrid.getCluster(clusterIdx);
        for(int iLoop = 0; iLoop < csCluster.getServerList().size(); iLoop ++) {
            AppServer cs = csCluster.getServer(iLoop);
            bRet = cs.start();
            if (bRet == false) {
                break;
            }
        }
        return bRet;
    }
    
    public boolean cleanOneCluster(AppGrid appGrid, int clusterIdx) {
        boolean bRet = false;
        AppCluster csCluster = appGrid.getCluster(clusterIdx);
        for(int iLoop = 0; iLoop < csCluster.getServerList().size(); iLoop ++) {
            AppServer cs = csCluster.getServer(iLoop);
            bRet = cs.stop(KillTypeEnum.NORMALKILL, WAIT_TIME);
            if (bRet == false) break;

            bRet = cs.clean();
            if (bRet == false) break;
        }
        return bRet;
    }
}